#include <alloca.h>
#include <ast.h>
#include <codegen.h>
#include <iostream>
#include <llvm-18/llvm/ADT/APFloat.h>
#include <llvm-18/llvm/IR/Constants.h>
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

llvm::Value *IdentifierNode::codegen() { return nullptr; }
llvm::Value *StringNode::codegen() { return nullptr; }
llvm::Value *ReturnNode::codegen() { return nullptr; }
llvm::Value *BreakNode::codegen() { return nullptr; }
llvm::Value *ContinueNode::codegen() { return nullptr; }
llvm::Value *ComparisonNode::codegen() { return nullptr; }
llvm::Value *IfNode::codegen() { return nullptr; }
llvm::Value *WhileNode::codegen() { return nullptr; }
llvm::Value *ForNode::codegen() { return nullptr; }
llvm::Value *FunctionNode::codegen() { return nullptr; }
llvm::Value *PrintNode::codegen() { return nullptr; }

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
