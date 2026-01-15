#include <alloca.h>
#include <ast.h>
#include <codegen.h>
#include <ios>
#include <iostream>
#include <llvm-18/llvm/ADT/APFloat.h>
#include <llvm-18/llvm/IR/Constants.h>
#include <llvm-18/llvm/IR/DataLayout.h>
#include <llvm-18/llvm/IR/Function.h>
#include <llvm-18/llvm/IR/IRBuilder.h>
#include <llvm-18/llvm/IR/Instructions.h>
#include <llvm-18/llvm/IR/Intrinsics.h>
#include <llvm-18/llvm/IR/Type.h>
#include <llvm-18/llvm/IR/Value.h>
#include <llvm-18/llvm/Support/Format.h>
#include <llvm/IR/Verifier.h>
#include <memory>

llvm::Value *NumberNode::codegen() {
  return llvm::ConstantFP::get(llvm::Type::getDoubleTy(*TheContext), number);
}

llvm::Value *VariableNode::codegen() {
  llvm::Value *v = NamedValues[name];
  if (!v)
    LogErrorV("Unknow Variable Name");
  return v;
}

llvm::Value *BinaryOperationNode::codegen() {
  llvm::Value *LHS = left->codegen();
  llvm::Value *RHS = right->codegen();

  if (!LHS || !RHS)
    return nullptr;
  switch (op) {

  case '+':
    return Builder->CreateFAdd(LHS, RHS, "addtmp");
  case '-':
    return Builder->CreateFSub(LHS, RHS, "subtmp");
  case '*':
    return Builder->CreateFMul(LHS, RHS, "multmp");
  case '<':
    LHS = Builder->CreateFCmpULT(LHS, RHS, "cmptmp");
    // Convert bool 0/1 to double 0.0 or 1.0
    return Builder->CreateUIToFP(LHS, llvm::Type::getDoubleTy(*TheContext),
                                 "booltmp");
  default:
    return LogErrorV("invalid binary operator");
  }
}

llvm::Value *VariableDeclareNode::codegen() {
  llvm::Type *Type = nullptr;

  if (type == "STRING") {
    Type = llvm::Type::getInt8Ty(*TheContext);
  } else if (type == "INTEGER") {
    Type = llvm::Type::getInt32Ty(*TheContext);
  } else if (type == "FLOAT") {
    Type = llvm::Type::getFloatTy(*TheContext);
  } else {
    LogErrorV("INVALID VARIABLE DELACRATION TYPE");
  }

  llvm::Function *func = Builder->GetInsertBlock()->getParent();
  llvm::IRBuilder<> tmpbuilder(&func->getEntryBlock(),
                               func->getEntryBlock().begin());
  llvm::AllocaInst *alloca = tmpbuilder.CreateAlloca(Type, nullptr, name);

  // Initialize if there is an initializer
  if (contents) {
    llvm::Value *initVal = contents->codegen();
    if (!initVal)
      return nullptr;
    Builder->CreateStore(initVal, alloca);
  }

  NamedValues[name] = alloca;
  return alloca;
}

llvm::Value *AssignmentNode::codegen() {
  llvm::Value *var = NamedValues[name->name];
  if (!var) {
    std::cerr << "Unknown variable name: " << name->name << std::endl;
    return nullptr;
  }

  llvm::Value *val = value->codegen();
  if (!val)
    return nullptr;

  Builder->CreateStore(val, var);
  return val;
}

// llvm::Value *IdentifierNode::codegen() {} DON"T NEED IT. I THINK...

llvm::Value *StringNode::codegen() {
  return Builder->CreateGlobalStringPtr(value);
}

llvm::Value *ReturnNode::codegen() {
  llvm::Value *V = expr ? expr->codegen() : nullptr;
  return Builder->CreateRet(V);
}

llvm::Value *BreakNode::codegen() { return Builder->CreateBr(CurrentLoopEnd); }

llvm::Value *ContinueNode::codegen() {
  return Builder->CreateBr(CurrentLoopStart);
}

llvm::Value *ComparisonNode::codegen() {

  llvm::Value *L = left->codegen();
  llvm::Value *R = right->codegen();

  if (!L || !R) {
    return nullptr;
  }

  llvm::Type *Type = nullptr;

  if (comp == "==")
    return Builder->CreateICmpEQ(L, R, "cmptmp");
  else if (comp == "!=")
    return Builder->CreateICmpNE(L, R, "cmptmp");
  else if (comp == "<")
    return Builder->CreateICmpSLT(L, R, "cmptmp");
  else if (comp == "<=")
    return Builder->CreateICmpSLE(L, R, "cmptmp");
  else if (comp == ">")
    return Builder->CreateICmpSGT(L, R, "cmptmp");
  else if (comp == ">=")
    return Builder->CreateICmpSGE(L, R, "cmptmp");

  return LogErrorV("Unknown comparison operator");
}

llvm::Value *IfNode::codegen() {
  llvm::Value *CondV = condition->codegen();
  if (!CondV)
    return nullptr;

  CondV = Builder->CreateICmpNE(
      CondV, llvm::ConstantInt::get(CondV->getType(), 0), "ifcond");

  llvm::Function *TheFunction = Builder->GetInsertBlock()->getParent();

  llvm::BasicBlock *ThenBB =
      llvm::BasicBlock::Create(*TheContext, "then", TheFunction);
  llvm::BasicBlock *MergeBB =
      llvm::BasicBlock::Create(*TheContext, "ifcont", TheFunction);
  llvm::BasicBlock *ElseBB =
      !elseBody.empty()
          ? llvm::BasicBlock::Create(*TheContext, "else", TheFunction)
          : nullptr;

  if (ElseBB)
    Builder->CreateCondBr(CondV, ThenBB, ElseBB);
  else
    Builder->CreateCondBr(CondV, ThenBB, MergeBB);

  Builder->SetInsertPoint(ThenBB);
  for (auto &stmt : body)
    stmt->codegen();
  Builder->CreateBr(MergeBB);

  if (ElseBB) {
    Builder->SetInsertPoint(ElseBB);
    for (auto &stmt : elseBody)
      stmt->codegen();
    Builder->CreateBr(MergeBB);
  }

  Builder->SetInsertPoint(MergeBB);
  return nullptr;
}

llvm::Value *WhileNode::codegen() {
  llvm::Function *TheFunction = Builder->GetInsertBlock()->getParent();

  llvm::BasicBlock *CondBB =
      llvm::BasicBlock::Create(*TheContext, "whilecond", TheFunction);
  llvm::BasicBlock *LoopBB =
      llvm::BasicBlock::Create(*TheContext, "loop", TheFunction);
  llvm::BasicBlock *AfterBB =
      llvm::BasicBlock::Create(*TheContext, "afterloop", TheFunction);

  // branch to condition first
  Builder->CreateBr(CondBB);

  // condition block
  Builder->SetInsertPoint(CondBB);
  llvm::Value *CondV = condition->codegen();
  if (!CondV)
    return nullptr;
  CondV = Builder->CreateICmpNE(
      CondV, llvm::ConstantInt::get(CondV->getType(), 0), "whilecond");

  Builder->CreateCondBr(CondV, LoopBB, AfterBB);

  // loop block
  Builder->SetInsertPoint(LoopBB);
  llvm::BasicBlock *PrevLoopStart = CurrentLoopStart;
  llvm::BasicBlock *PrevLoopEnd = CurrentLoopEnd;
  CurrentLoopStart = CondBB;
  CurrentLoopEnd = AfterBB;

  for (auto &stmt : body)
    stmt->codegen();

  Builder->CreateBr(CondBB); // back to condition

  // restore previous loop context
  CurrentLoopStart = PrevLoopStart;
  CurrentLoopEnd = PrevLoopEnd;

  // after loop
  Builder->SetInsertPoint(AfterBB);

  return nullptr;
}

// llvm::Value *ForNode::codegen() {} GONNA IMPLEMENT IT LATER
// llvm::Value *FunctionNode::codegen() { return nullptr; } THIS TOO
llvm::Value *PrintNode::codegen() {
  llvm::Value *Val = args->codegen();
  if (!Val)
    return nullptr;

  // declare printf if not already
  llvm::Function *PrintF = TheModule->getFunction("printf");
  if (!PrintF) {
    std::vector<llvm::Type *> printfArgs;
    printfArgs.push_back(Builder->getInt8Ty());
    llvm::FunctionType *printfType =
        llvm::FunctionType::get(Builder->getInt32Ty(), printfArgs, true);
    PrintF = llvm::Function::Create(printfType, llvm::Function::ExternalLinkage,
                                    "printf", TheModule.get());
  }

  // create format string for int
  llvm::Value *formatStr = Builder->CreateGlobalStringPtr("%d\n");
  return Builder->CreateCall(PrintF, {formatStr, Val}, "printfCall");
}

int main() {
  TheContext = std::make_unique<llvm::LLVMContext>();
  TheModule = std::make_unique<llvm::Module>("my_module", *TheContext);
  Builder = std::make_unique<llvm::IRBuilder<>>(*TheContext);

  auto *FT =
      llvm::FunctionType::get(llvm::Type::getDoubleTy(*TheContext), false);
  auto *F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, "main",
                                   TheModule.get());
  auto *BB = llvm::BasicBlock::Create(*TheContext, "entry", F);
  Builder->SetInsertPoint(BB);

  auto code = std::make_unique<VariableDeclareNode>(
      "hello", "INTEGER",
      std::make_unique<BinaryOperationNode>(std::make_unique<NumberNode>(23),
                                            std::make_unique<NumberNode>(33),
                                            '+'));

  llvm::Value *Result = code->codegen();
  Builder->CreateRet(Result);

  llvm::verifyFunction(*F, &llvm::errs());
  TheModule->print(llvm::outs(), nullptr);
}
