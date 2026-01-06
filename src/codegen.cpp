#include "codegen.h"
#include "ast.h"

std::unique_ptr<llvm::LLVMContext> TheContext;
std::unique_ptr<llvm::Module> TheModule;
std::unique_ptr<llvm::IRBuilder<>> Builder;
std::unordered_map<std::string, llvm::Value *> NamedValues;


#include "codegen.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Type.h"
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>

// These should be declared in codegen.h or as extern in codegen.cpp
extern std::unique_ptr<llvm::LLVMContext> TheContext;
extern std::unique_ptr<llvm::Module> TheModule;
extern std::unique_ptr<llvm::IRBuilder<>> Builder;
extern std::unordered_map<std::string, llvm::AllocaInst*> NamedValues;

// ------------------ Expression / Value Nodes ------------------

llvm::Value* NumberNode::codegen() {
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), number, true);
}

llvm::Value* VariableNode::VariableDeclareNode::codegen() {
    llvm::Type *llvmType = nullptr;
    if (type == "int")
        llvmType = llvm::Type::getInt32Ty(*TheContext);
    else if (type == "float")
        llvmType = llvm::Type::getFloatTy(*TheContext);
    else
        return nullptr;

    llvm::Function *func = Builder->GetInsertBlock()->getParent();
    llvm::IRBuilder<> TmpB(&func->getEntryBlock(), func->getEntryBlock().begin());
    llvm::AllocaInst *Alloca = TmpB.CreateAlloca(llvmType, nullptr, name);

    NamedValues[name] = Alloca;
    return Alloca;
}

llvm::Value* VariableDeclareNode::codegen() {
    llvm::Type *llvmType = nullptr;

    if (type == "INT")
        llvmType = llvm::Type::getInt32Ty(*TheContext);
    else if (type == "FLOAT")
        llvmType = llvm::Type::getFloatTy(*TheContext);
    else if (type == "STRING")
        llvmType = llvm::Type::getInt8Ty(*TheContext);
    else
        return nullptr;

    llvm::Function *func = Builder->GetInsertBlock()->getParent();
    llvm::IRBuilder<> TmpB(&func->getEntryBlock(), func->getEntryBlock().begin());
    llvm::AllocaInst *Alloca = TmpB.CreateAlloca(llvmType, nullptr, name);

    NamedValues[name] = Alloca;
    return Alloca;
}

llvm::Value* AssignmentNode::codegen() {
    if (value.empty())
        return nullptr;

    llvm::Value *val = value[0]->codegen();
    if (!val)
        return nullptr;

    auto it = NamedValues.find(name->name);
    if (it == NamedValues.end()) {
        fprintf(stderr, "Unknown variable: %s\n", name->name.c_str());
        return nullptr;
    }

    return Builder->CreateStore(val, it->second);
}

llvm::Value* BinaryOperationNode::codegen() {
    llvm::Value *L = left->codegen();
    llvm::Value *R = right->codegen();
    if (!L || !R)
        return nullptr;

    if (op == "+") {
        if (L->getType()->isIntegerTy())
            return Builder->CreateAdd(L, R, "addtmp");
        else
            return Builder->CreateFAdd(L, R, "addtmp");
    }
    if (op == "-") {
        if (L->getType()->isIntegerTy())
            return Builder->CreateSub(L, R, "subtmp");
        else
            return Builder->CreateFSub(L, R, "subtmp");
    }
    if (op == "*") {
        if (L->getType()->isIntegerTy())
            return Builder->CreateMul(L, R, "multmp");
        else
            return Builder->CreateFMul(L, R, "multmp");
    }
    if (op == "/") {
        if (L->getType()->isIntegerTy())
            return Builder->CreateSDiv(L, R, "divtmp");
        else
            return Builder->CreateFDiv(L, R, "divtmp");
    }

    fprintf(stderr, "Unknown binary operator: %s\n", op.c_str());
    return nullptr;
}

llvm::Value* IdentifierNode::codegen() {
    auto it = NamedValues.find(id);
    if (it == NamedValues.end()) {
        fprintf(stderr, "Unknown variable: %s\n", id.c_str());
        return nullptr;
    }
    return Builder->CreateLoad(it->second->getAllocatedType(), it->second, id.c_str());
}

llvm::Value* StringNode::codegen() {
    return Builder->CreateGlobalStringPtr(value, "str");
}

// ------------------ Control Flow Nodes ------------------

llvm::Value* ReturnNode::codegen() {
    if (!expr)
        return Builder->CreateRetVoid();
    llvm::Value *val = expr->codegen();
    if (!val)
        return nullptr;
    return Builder->CreateRet(val);
}

llvm::Value* ComparisonNode::codegen() {
    if (left.empty() || right.empty())
        return nullptr;

    llvm::Value *L = left[0]->codegen();
    llvm::Value *R = right[0]->codegen();
    if (!L || !R)
        return nullptr;

    if (comp == "==")
        return Builder->CreateICmpEQ(L, R, "cmptmp");
    if (comp == "!=")
        return Builder->CreateICmpNE(L, R, "cmptmp");
    if (comp == "<")
        return Builder->CreateICmpSLT(L, R, "cmptmp");
    if (comp == "<=")
        return Builder->CreateICmpSLE(L, R, "cmptmp");
    if (comp == ">")
        return Builder->CreateICmpSGT(L, R, "cmptmp");
    if (comp == ">=")
        return Builder->CreateICmpSGE(L, R, "cmptmp");

    return nullptr;
}

llvm::Value* IfNode::codegen() {
    if (condition.empty() || !condition[0])
        return nullptr;

    llvm::Value *CondV = condition[0]->codegen();
    if (!CondV)
        return nullptr;

    CondV = Builder->CreateICmpNE(CondV, llvm::ConstantInt::get(CondV->getType(), 0), "ifcond");

    llvm::Function *TheFunction = Builder->GetInsertBlock()->getParent();

    llvm::BasicBlock *ThenBB = llvm::BasicBlock::Create(*TheContext, "then", TheFunction);
    llvm::BasicBlock *ElseBB = llvm::BasicBlock::Create(*TheContext, "else");
    llvm::BasicBlock *MergeBB = llvm::BasicBlock::Create(*TheContext, "ifcont");

    if (elseBody.empty())
        Builder->CreateCondBr(CondV, ThenBB, MergeBB);
    else
        Builder->CreateCondBr(CondV, ThenBB, ElseBB);

    Builder->SetInsertPoint(ThenBB);
    for (auto &stmt : body)
        stmt->codegen();
    Builder->CreateBr(MergeBB);

    if (!elseBody.empty()) {
        TheFunction->getBasicBlockList().push_back(ElseBB);
        Builder->SetInsertPoint(ElseBB);
        for (auto &stmt : elseBody)
            stmt->codegen();
        Builder->CreateBr(MergeBB);
    }

    TheFunction->getBasicBlockList().push_back(MergeBB);
    Builder->SetInsertPoint(MergeBB);

    return llvm::Constant::getNullValue(llvm::Type::getInt32Ty(*TheContext));
}

llvm::Value* WhileNode::codegen() {
    llvm::Function *TheFunction = Builder->GetInsertBlock()->getParent();

    llvm::BasicBlock *CondBB = llvm::BasicBlock::Create(*TheContext, "loopcond", TheFunction);
    llvm::BasicBlock *LoopBB = llvm::BasicBlock::Create(*TheContext, "loop", TheFunction);
    llvm::BasicBlock *AfterBB = llvm::BasicBlock::Create(*TheContext, "afterloop", TheFunction);

    Builder->CreateBr(CondBB);
    Builder->SetInsertPoint(CondBB);
    llvm::Value *CondV = condition->codegen();
    if (!CondV)
        return nullptr;
    CondV = Builder->CreateICmpNE(CondV, llvm::ConstantInt::get(CondV->getType(), 0), "whilecond");
    Builder->CreateCondBr(CondV, LoopBB, AfterBB);

    Builder->SetInsertPoint(LoopBB);
    for (auto &stmt : body)
        stmt->codegen();
    Builder->CreateBr(CondBB);

    Builder->SetInsertPoint(AfterBB);
    return llvm::Constant::getNullValue(llvm::Type::getInt32Ty(*TheContext));
}

llvm::Value* ForNode::codegen() {
    if (init)
        init->codegen();

    llvm::Function *TheFunction = Builder->GetInsertBlock()->getParent();

    llvm::BasicBlock *CondBB = llvm::BasicBlock::Create(*TheContext, "forcond", TheFunction);
    llvm::BasicBlock *LoopBB = llvm::BasicBlock::Create(*TheContext, "forloop", TheFunction);
    llvm::BasicBlock *AfterBB = llvm::BasicBlock::Create(*TheContext, "afterfor", TheFunction);

    Builder->CreateBr(CondBB);
    Builder->SetInsertPoint(CondBB);

    llvm::Value *CondV = nullptr;
    if (condition)
        CondV = condition->codegen();
    if (!CondV)
        CondV = llvm::ConstantInt::getTrue(*TheContext);

    CondV = Builder->CreateICmpNE(CondV, llvm::ConstantInt::get(CondV->getType(), 0), "forcond");
    Builder->CreateCondBr(CondV, LoopBB, AfterBB);

    Builder->SetInsertPoint(LoopBB);
    for (auto &stmt : body)
        stmt->codegen();

    if (increment)
        increment->codegen();
    Builder->CreateBr(CondBB);

    Builder->SetInsertPoint(AfterBB);
    return llvm::Constant::getNullValue(llvm::Type::getInt32Ty(*TheContext));
}

llvm::Value* FunctionNode::codegen() {
    std::vector<llvm::Type*> ArgTypes(parameters.size(), llvm::Type::getInt32Ty(*TheContext));
    llvm::FunctionType *FT = llvm::FunctionType::get(llvm::Type::getInt32Ty(*TheContext), ArgTypes, false);
    llvm::Function *F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, name, TheModule.get());

    unsigned Idx = 0;
    for (auto &Arg : F->args())
        Arg.setName(parameters[Idx++]);

    llvm::BasicBlock *BB = llvm::BasicBlock::Create(*TheContext, "entry", F);
    Builder->SetInsertPoint(BB);

    NamedValues.clear();
    Idx = 0;
    for (auto &Arg : F->args()) {
        llvm::AllocaInst *Alloca = Builder->CreateAlloca(llvm::Type::getInt32Ty(*TheContext), nullptr, Arg.getName());
        Builder->CreateStore(&Arg, Alloca);
        NamedValues[Arg.getName().str()] = Alloca;
    }

    for (auto &stmt : body)
        stmt->codegen();

    llvm::verifyFunction(*F);
    return F;
}

llvm::Value* PrintNode::codegen() {
    llvm::Value *val = expr->codegen();
    if (!val)
        return nullptr;

    llvm::Function *printfFunc = TheModule->getFunction("printf");
    if (!printfFunc) {
        std::vector<llvm::Type *> printfArgs;
        printfArgs.push_back(Builder->getInt8Ty()->getPointerTo());
        llvm::FunctionType *printfType = llvm::FunctionType::get(Builder->getInt32Ty(), printfArgs, true);
        printfFunc = llvm::Function::Create(printfType, llvm::Function::ExternalLinkage, "printf", TheModule.get());
    }

    llvm::Value *formatStr = nullptr;
    if (val->getType()->isIntegerTy(32))
        formatStr = Builder->CreateGlobalStringPtr("%d\n");
    else if (val->getType()->isFloatTy())
        formatStr = Builder->CreateGlobalStringPtr("%f\n");
    else if (val->getType()->isPointerTy())
        formatStr = Builder->CreateGlobalStringPtr("%s\n");
    else
        return nullptr;

    return Builder->CreateCall(printfFunc, {formatStr, val}, "printfCall");
}

