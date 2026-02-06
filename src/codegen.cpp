#include "lexer.h"
#include <ast.h>
#include <iostream>
#include <llvm-18/llvm/IR/BasicBlock.h>
#include <llvm-18/llvm/IR/Constants.h>
#include <llvm-18/llvm/IR/DerivedTypes.h>
#include <llvm-18/llvm/IR/Function.h>
#include <llvm-18/llvm/IR/Instructions.h>
#include <llvm-18/llvm/IR/LLVMContext.h>
#include <llvm-18/llvm/IR/Type.h>
#include <llvm-18/llvm/IR/Value.h>
#include <llvm-18/llvm/IR/Verifier.h>
#include <llvm-18/llvm/Support/TypeName.h>
#include <llvm-18/llvm/Support/raw_ostream.h>
#include <memory>
#include <vector>

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

llvm::Type *GetTypeVoid(TokenType type, llvm::LLVMContext &context) {
  if (type == TokenType::VOID)
    return llvm::Type::getVoidTy(context);

  return GetTypeNonVoid(type, context);
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

llvm::Value *AssignmentNode::codegen(CodegenContext &cc) {
  llvm::Value *var = cc.lookup(name);
  if (!var) {
    llvm::errs() << "Error: variable '" << name << "' not declared!\n";
    return nullptr; // prevents cast crash
  }

  llvm::Value *valueVal = val->codegen(cc);
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

  // cc.popScope(); // exit scope
  return last; // may be nullptr if all statements failed
}

llvm::Value *FunctionNode::codegen(CodegenContext &cc) {
  // arg types
  std::vector<llvm::Type *> argTypes;
  for (auto &a : args)
    argTypes.push_back(a.second);

  // function type
  llvm::Type *retTy = GetTypeVoid(ReturnType, *cc.TheContext);
  auto *FT = llvm::FunctionType::get(retTy, argTypes, false);

  // function
  auto *Fn = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, name,
                                    cc.Module.get());

  // entry block
  auto *BB = llvm::BasicBlock::Create(*cc.TheContext, "entry", Fn);
  cc.Builder->SetInsertPoint(BB);

  cc.pushScope();

  // arguments → allocas
  unsigned i = 0;
  for (auto &arg : Fn->args()) {
    const auto &argName = args[i++].first;
    arg.setName(argName);

    auto *alloca = cc.Builder->CreateAlloca(arg.getType(), nullptr, argName);
    cc.Builder->CreateStore(&arg, alloca);
    cc.addVariable(argName, alloca);
  }

  // body
  llvm::Value *retVal = content->codegen(cc);

  // emit return only if missing
  if (!BB->getTerminator()) {
    if (retTy->isVoidTy()) {
      cc.Builder->CreateRetVoid();
    } else {
      if (!retVal) {
        Fn->eraseFromParent();
        cc.popScope();
        return nullptr;
      }
      cc.Builder->CreateRet(retVal);
    }
  }

  llvm::verifyFunction(*Fn);
  cc.popScope();
  return Fn;
}

llvm::Value *VariableReferenceNode::codegen(CodegenContext &cc) {
  llvm::Value *var = cc.lookup(Name);
  if (!var)
    throw std::runtime_error("Unknown variable: " + Name);

  llvm::Type *type;
  if (auto *AI = llvm::dyn_cast<llvm::AllocaInst>(var)) {
    type = AI->getAllocatedType(); // safe in LLVM 18+
  } else {
    throw std::runtime_error("Variable is not an alloca: " + Name);
  }

  return cc.Builder->CreateLoad(type, var, Name);
}

llvm::Value *WhileNode::codegen(CodegenContext &cc) {
  llvm::Function *F = cc.Builder->GetInsertBlock()->getParent();
  llvm::LLVMContext &Ctx = *cc.TheContext;

  // create all blocks with the function as parent (no manual push_back needed)
  llvm::BasicBlock *condBB = llvm::BasicBlock::Create(Ctx, "while.cond", F);
  llvm::BasicBlock *bodyBB = llvm::BasicBlock::Create(Ctx, "while.body", F);
  llvm::BasicBlock *afterBB = llvm::BasicBlock::Create(Ctx, "while.end", F);

  // jump to condition
  cc.Builder->CreateBr(condBB);

  // --- condition block ---
  cc.Builder->SetInsertPoint(condBB);
  llvm::Value *condVal = condition->codegen(cc);
  if (!condVal)
    return nullptr;

  // If condition is not already i1, compare against zero to produce i1.
  if (!condVal->getType()->isIntegerTy(1)) {
    // handle vector/scalar integers (assumes integer scalar type)
    condVal = cc.Builder->CreateICmpNE(
        condVal, llvm::ConstantInt::get(condVal->getType(), 0),
        "while.cond.to.i1");
  }
  cc.Builder->CreateCondBr(condVal, bodyBB, afterBB);

  // --- body block ---
  cc.Builder->SetInsertPoint(bodyBB);

  // save/restore break & continue targets (if your context tracks them)
  llvm::BasicBlock *oldBreak = cc.BreakBB;
  llvm::BasicBlock *oldCont = cc.ContinueBB;
  cc.BreakBB = afterBB;
  cc.ContinueBB = condBB;

  if (!body->codegen(cc)) {
    cc.BreakBB = oldBreak;
    cc.ContinueBB = oldCont;
    return nullptr;
  }

  // restore break/continue
  cc.BreakBB = oldBreak;
  cc.ContinueBB = oldCont;

  // if body didn't terminate (no return/branch), jump back to cond
  if (!cc.Builder->GetInsertBlock()->getTerminator())
    cc.Builder->CreateBr(condBB);

  // --- after loop ---
  cc.Builder->SetInsertPoint(afterBB);

  // while as a statement: return an arbitrary zero value (adapt to your Node
  // API)
  return llvm::Constant::getNullValue(llvm::Type::getInt32Ty(Ctx));
}

int main() {
  CodegenContext ctx("myprogram");
  ctx.pushScope(); // Start Global Scope

  std::vector<std::unique_ptr<ast>> vals;

  vals.push_back(std::make_unique<VariableDeclareNode>(
      "val1", std::make_unique<IntegerNode>(21), TokenType::INTEGER));
  vals.push_back(std::make_unique<VariableDeclareNode>(
      "val2", std::make_unique<VariableReferenceNode>("val1"),
      TokenType::INTEGER));
  vals.push_back(std::make_unique<WhileNode>(
      std::make_unique<VariableReferenceNode>("val2"),
      std::make_unique<AssignmentNode>("val1",
                                       std::make_unique<IntegerNode>(21))));

  auto Compound = std::make_unique<CompoundNode>(std::move(vals));

  std::vector<std::pair<std::string, llvm::Type *>> type;

  auto Function = std::make_unique<FunctionNode>(
      "main", type, std::move(Compound), TokenType::VOID);

  Function->codegen(ctx);

  ctx.Module->print(llvm::errs(), nullptr);
  ctx.popScope(); // End Global Scope
  return 0;
}
