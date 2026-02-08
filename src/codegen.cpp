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
#include <stdexcept>
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

llvm::Value *ReturnNode::codegen(CodegenContext &cc) {
  if (expr) {
    llvm::Value *retVal = expr->codegen(cc);
    return cc.Builder->CreateRet(retVal);
  } else {
    return cc.Builder->CreateRetVoid();
  }
}

// llvm::Value *CompoundNode::codegen(CodegenContext &cc) {
//   llvm::Value *last = nullptr;

//   cc.pushScope();

//   for (auto &stmt : blocks) {
//     if (!stmt)
//       continue;
//     llvm::Value *val = stmt->codegen(cc);
//     if (!val) {
//       llvm::errs() << "Warning: statement returned null in CompoundNode\n";
//     }
//     last = val;
//   }

//   // cc.popScope();
//   return last;
// }

llvm::Value *CompoundNode::codegen(CodegenContext &cc) {
  llvm::Value *last = nullptr;

  cc.pushScope();

  for (auto &stmt : blocks) {
    if (!stmt)
      continue;

    llvm::Value *val = stmt->codegen(cc);
    if (!val) {
      llvm::errs() << "Warning: statement returned null in CompoundNode\n";
      continue; // skip nulls
    }

    last = val;

    // Only check for return if val is not null
    if (llvm::isa<llvm::ReturnInst>(val)) {
      break;
    }
  }

  cc.popScope();
  return last;
}

llvm::Value *FunctionNode::codegen(CodegenContext &cc) {
  std::vector<llvm::Type *> argTypes;
  for (auto &a : args)
    argTypes.push_back(a.second);

  llvm::Type *retTy = GetTypeVoid(ReturnType, *cc.TheContext);
  auto *FT = llvm::FunctionType::get(retTy, argTypes, false);

  auto *Fn = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, name,
                                    cc.Module.get());

  auto *BB = llvm::BasicBlock::Create(*cc.TheContext, "entry", Fn);
  cc.Builder->SetInsertPoint(BB);

  cc.pushScope();

  unsigned i = 0;
  for (auto &arg : Fn->args()) {
    const auto &argName = args[i++].first;
    arg.setName(argName);

    auto *alloca = cc.Builder->CreateAlloca(arg.getType(), nullptr, argName);
    cc.Builder->CreateStore(&arg, alloca);
    cc.addVariable(argName, alloca);
  }

  llvm::Value *retVal = content->codegen(cc);

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

  llvm::BasicBlock *condBB = llvm::BasicBlock::Create(Ctx, "while.cond", F);
  llvm::BasicBlock *bodyBB = llvm::BasicBlock::Create(Ctx, "while.body", F);
  llvm::BasicBlock *afterBB = llvm::BasicBlock::Create(Ctx, "while.end", F);

  cc.Builder->CreateBr(condBB);

  cc.Builder->SetInsertPoint(condBB);
  llvm::Value *condVal = condition->codegen(cc);
  if (!condVal)
    return nullptr;

  if (!condVal->getType()->isIntegerTy(1)) {
    condVal = cc.Builder->CreateICmpNE(
        condVal, llvm::ConstantInt::get(condVal->getType(), 0),
        "while.cond.to.i1");
  }
  cc.Builder->CreateCondBr(condVal, bodyBB, afterBB);

  cc.Builder->SetInsertPoint(bodyBB);

  llvm::BasicBlock *oldBreak = cc.BreakBB;
  llvm::BasicBlock *oldCont = cc.ContinueBB;
  cc.BreakBB = afterBB;
  cc.ContinueBB = condBB;

  if (!body->codegen(cc)) {
    cc.BreakBB = oldBreak;
    cc.ContinueBB = oldCont;
    return nullptr;
  }

  cc.BreakBB = oldBreak;
  cc.ContinueBB = oldCont;

  if (!cc.Builder->GetInsertBlock()->getTerminator())
    cc.Builder->CreateBr(condBB);

  cc.Builder->SetInsertPoint(afterBB);
  return llvm::Constant::getNullValue(llvm::Type::getInt32Ty(Ctx));
}

llvm::Value *IfNode::codegen(CodegenContext &cc) {
  llvm::Value *condV = condition->codegen(cc);
  if (!condV)
    return nullptr;

  // bool conversion
  condV = cc.Builder->CreateICmpNE(
      condV, llvm::ConstantInt::get(condV->getType(), 0), "ifcond");

  llvm::Function *func = cc.Builder->GetInsertBlock()->getParent();

  // create blocks
  llvm::BasicBlock *thenBB =
      llvm::BasicBlock::Create(*cc.TheContext, "then", func);
  llvm::BasicBlock *elseBB = nullptr;
  if (elseBlock)
    elseBB = llvm::BasicBlock::Create(*cc.TheContext, "else", func);

  llvm::BasicBlock *mergeBB =
      llvm::BasicBlock::Create(*cc.TheContext, "ifcont", func);

  // conditional branch
  if (elseBB)
    cc.Builder->CreateCondBr(condV, thenBB, elseBB);
  else
    cc.Builder->CreateCondBr(condV, thenBB, mergeBB);

  // then
  cc.Builder->SetInsertPoint(thenBB);
  cc.pushScope();
  thenBlock->codegen(cc);
  cc.popScope();
  cc.Builder->CreateBr(mergeBB);
  thenBB = cc.Builder->GetInsertBlock();

  // else (if present)
  if (elseBB) {
    cc.Builder->SetInsertPoint(elseBB);
    cc.pushScope();
    elseBlock->codegen(cc);
    cc.popScope();
    cc.Builder->CreateBr(mergeBB);
    elseBB = cc.Builder->GetInsertBlock();
  }

  // merge
  cc.Builder->SetInsertPoint(mergeBB);
  return nullptr;
}

llvm::Value *BinaryOperationNode::codegen(CodegenContext &cc) {
  llvm::Value *LHS = Left->codegen(cc);
  llvm::Value *RHS = Right->codegen(cc);

  if (!LHS || !RHS)
    throw std::runtime_error("null operand in binary operation");

  // force both operands to i32
  auto *i32 = llvm::Type::getInt32Ty(*cc.TheContext);

  if (LHS->getType()->isIntegerTy(1))
    LHS = cc.Builder->CreateIntCast(LHS, i32, true);

  if (RHS->getType()->isIntegerTy(1))
    RHS = cc.Builder->CreateIntCast(RHS, i32, true);

  switch (Type) {
  case TokenType::PLUS:
    return cc.Builder->CreateAdd(LHS, RHS, "addtmp");

  case TokenType::MINUS:
    return cc.Builder->CreateSub(LHS, RHS, "subtmp");

  case TokenType::STAR:
    return cc.Builder->CreateMul(LHS, RHS, "multmp");

  case TokenType::SLASH:
    return cc.Builder->CreateSDiv(LHS, RHS, "divtmp");

  case TokenType::LT: {
    auto *cmp = cc.Builder->CreateICmpSLT(LHS, RHS, "lttmp");
    return cc.Builder->CreateIntCast(cmp, i32, true);
  }

  case TokenType::LTE: {
    auto *cmp = cc.Builder->CreateICmpSLE(LHS, RHS, "letmp");
    return cc.Builder->CreateIntCast(cmp, i32, true);
  }

  case TokenType::GT: {
    auto *cmp = cc.Builder->CreateICmpSGT(LHS, RHS, "gttmp");
    return cc.Builder->CreateIntCast(cmp, i32, true);
  }

  case TokenType::GTE: {
    auto *cmp = cc.Builder->CreateICmpSGE(LHS, RHS, "getmp");
    return cc.Builder->CreateIntCast(cmp, i32, true);
  }

  case TokenType::EQEQ: {
    auto *cmp = cc.Builder->CreateICmpEQ(LHS, RHS, "eqtmp");
    return cc.Builder->CreateIntCast(cmp, i32, true);
  }

  case TokenType::NOTEQ: {
    auto *cmp = cc.Builder->CreateICmpNE(LHS, RHS, "netmp");
    return cc.Builder->CreateIntCast(cmp, i32, true);
  }

  default:
    throw std::runtime_error("unknown binary operator");
  }
}

llvm::Value *BreakNode::codegen(CodegenContext &cc) {
  if (!cc.BreakBB) {
    std::cerr << "Error: 'break' not inside a loop.\n";
    return nullptr;
  }
  return cc.Builder->CreateBr(cc.BreakBB);
}

llvm::Value *CallNode::codegen(CodegenContext &cc) {
  llvm::Function *callee = cc.Module->getFunction(name);
  if (!callee)
    return nullptr;

  if (callee->arg_size() != args.size())
    return nullptr;

  std::vector<llvm::Value *> argVals;
  for (auto &arg : args) {
    llvm::Value *v = arg->codegen(cc);
    if (!v)
      return nullptr;
    argVals.push_back(v);
  }

  return cc.Builder->CreateCall(callee, argVals);
}

llvm::Value *ContinueNode::codegen(CodegenContext &cc) {

  if (!cc.BreakBB) {
    std::cerr << "Error: 'break' not inside a loop.\n";
    return nullptr;
  }
  return cc.Builder->CreateBr(cc.ContinueBB);
}

// int main() {
//   CodegenContext ctx("myprogram");
//   ctx.pushScope(); // Start Global Scope

//   // --- First compound for "random" function ---
//   std::vector<std::unique_ptr<ast>> vals;

//   vals.push_back(std::make_unique<VariableDeclareNode>(
//       "val2", std::make_unique<VariableReferenceNode>("val1"),
//       TokenType::INTEGER));

//   vals.push_back(std::make_unique<WhileNode>(
//       std::make_unique<VariableReferenceNode>("val2"),
//       std::make_unique<ContinueNode>()));

//   vals.push_back(std::make_unique<IfNode>(
//       std::make_unique<VariableReferenceNode>("val2"),
//       std::make_unique<IntegerNode>(21), std::make_unique<IntegerNode>(32)));

//   vals.push_back(
//       std::make_unique<ReturnNode>(std::make_unique<BinaryOperationNode>(
//           TokenType::GTE, std::make_unique<VariableReferenceNode>("val1"),
//           std::make_unique<VariableReferenceNode>("val2"))));

//   auto compoundRandom = std::make_unique<CompoundNode>(std::move(vals));

//   std::vector<std::pair<std::string, llvm::Type *>> typeRandom = {
//       {"val1", llvm::Type::getInt32Ty(*ctx.TheContext)}};

//   auto RandomFunction = std::make_unique<FunctionNode>(
//       "random", typeRandom, std::move(compoundRandom), TokenType::BOOLEAN);

//   // --- Second compound for "main" function ---
//   std::vector<std::unique_ptr<ast>> anothervals;

//   anothervals.push_back(std::make_unique<VariableDeclareNode>(
//       "val1", std::make_unique<IntegerNode>(21), TokenType::INTEGER));

//   // Prepare arguments vector separately to move unique_ptrs
//   std::vector<std::unique_ptr<ast>> callArgs;
//   callArgs.push_back(std::make_unique<VariableReferenceNode>("val1"));

//   anothervals.push_back(
//       std::make_unique<CallNode>("random", std::move(callArgs)));

//   auto anotherCompound = std::make_unique<CompoundNode>(std::move(anothervals));

//   auto Function = std::make_unique<FunctionNode>(
//       "main", typeRandom, std::move(anotherCompound), TokenType::INTEGER);

//   // --- Codegen ---
//   RandomFunction->codegen(ctx);
//   Function->codegen(ctx);

//   ctx.Module->print(llvm::errs(), nullptr);

//   ctx.popScope(); // End Global Scope
//   return 0;
// }
