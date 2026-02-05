#include "lexer.h"
#include <ast.h>
#include <iostream>
#include <llvm-18/llvm/IR/Constants.h>
#include <llvm-18/llvm/IR/DerivedTypes.h>
#include <llvm-18/llvm/IR/Instructions.h>
#include <llvm-18/llvm/IR/LLVMContext.h>
#include <llvm-18/llvm/IR/Type.h>
#include <llvm-18/llvm/IR/Value.h>
#include <llvm-18/llvm/IR/Verifier.h>
#include <llvm-18/llvm/Support/raw_ostream.h>
#include <memory>

llvm::Type *GetTypeNonVoid(TokenType type, llvm::LLVMContext &context) {
  switch (type) {
  case (TokenType::INTEGER):
    return llvm::Type::getInt32Ty(context);
    break;
  case (TokenType::FLOAT):
    return llvm::Type::getFloatTy(context);
    break;
  case (TokenType::STRING):
    return llvm::PointerType::get(llvm::Type::getInt32Ty(context), false);
    break;
  case (TokenType::BOOLEAN):
    return llvm::Type::getInt1Ty(context);
    break;
  default:
    std::cerr << "Invalid Variable Type\n";
    return nullptr;
    break;
  }

  return nullptr;
}

llvm::Value *IntegerNode::codegen(CodegenContext &cc) {

  return llvm::ConstantInt::get(llvm::Type::getInt32Ty(*cc.TheContext), val,
                                true);
}
llvm::Value *FloatNode::codegen(CodegenContext &cc) {

  return llvm::ConstantFP::get(llvm::Type::getFloatTy(*cc.TheContext), val);
}
llvm::Value *BooleanNode::codegen(CodegenContext &cc) {
  return llvm::ConstantInt::get(llvm::Type::getInt1Ty(*cc.TheContext), val,
                                true);
}

llvm::Value *StringNode::codegen(CodegenContext &cc) {
  return cc.Builder->CreateGlobalStringPtr(val);
}

llvm::Value *VariableDeclareNode::codegen(CodegenContext &cc) {
  llvm::Type *llvmType = GetTypeNonVoid(Type, *cc.TheContext);
  llvm::AllocaInst *alloca = cc.Builder->CreateAlloca(llvmType, nullptr, name);

  if (val) {
    llvm::Value *initVal = val->codegen(cc);
    cc.Builder->CreateStore(initVal, alloca);
  }

  cc.addVariable(name, alloca);
  return alloca;
}

llvm::Value* AssignmentNode::codegen(CodegenContext &cc) {
    llvm::Value* var = cc.lookup(name);
    if (!var) {
        llvm::errs() << "Error: variable '" << name << "' not declared!\n";
        return nullptr;  // prevents cast crash
    }

    llvm::Value* valueVal = val->codegen(cc);
    if (!valueVal) {
        llvm::errs() << "Error: RHS expression returned null!\n";
        return nullptr;
    }

    // Store the value into the existing alloca
    return cc.Builder->CreateStore(valueVal, var);
}


llvm::Value *CompoundNode::codegen(CodegenContext &cc) {
  llvm::Value *last = nullptr;

  cc.pushScope(); // new scope for this block

  for (auto &stmt : blocks) {
    if (!stmt)
      continue; // skip null statements
    llvm::Value *val = stmt->codegen(cc);
    if (!val) {
      llvm::errs() << "Warning: statement returned null in CompoundNode\n";
    }
    last = val; // last non-null statement
  }

  cc.popScope(); // exit scope
  return last;   // may be nullptr if all statements failed
}

int main() {
  CodegenContext ctx("myprogram");
  ctx.pushScope(); // start global scope

  // Function type: int main()
  llvm::Type *intTy = llvm::Type::getInt32Ty(*ctx.TheContext);
  llvm::FunctionType *funcType = llvm::FunctionType::get(intTy, false);
  llvm::Function *mainFunc = llvm::Function::Create(
      funcType, llvm::Function::ExternalLinkage, "main", ctx.Module.get());

  // Entry basic block
  llvm::BasicBlock *entry =
      llvm::BasicBlock::Create(*ctx.TheContext, "entry", mainFunc);
  ctx.Builder->SetInsertPoint(entry);

  std::vector<std::unique_ptr<ast>> val;
  val.push_back(std::make_unique<VariableDeclareNode>(
      "i", std::make_unique<IntegerNode>(21), TokenType::INTEGER));
  val.push_back(
      std::make_unique<AssignmentNode>("i", std::make_unique<IntegerNode>(42)));

  auto compound = std::make_unique<CompoundNode>(std::move(val));
  compound->codegen(ctx); // execute the block

  // 3. Load variable and return it
  llvm::AllocaInst *alloca = llvm::cast<llvm::AllocaInst>(ctx.lookup("i"));
  llvm::Value *retVal =
      ctx.Builder->CreateLoad(alloca->getAllocatedType(), alloca);
  ctx.Builder->CreateRet(retVal);

  // 4. Verify and print module
  llvm::verifyModule(*ctx.Module, &llvm::errs());
  ctx.Module->print(llvm::outs(), nullptr);

  ctx.popScope(); // end global scope
  return 0;
}
