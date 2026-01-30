#include <ast.h>
#include <iostream>
#include <llvm-18/llvm/ADT/STLFunctionalExtras.h>
#include <llvm-18/llvm/IR/Constants.h>
#include <llvm-18/llvm/IR/DerivedTypes.h>
#include <llvm-18/llvm/IR/Function.h>
#include <llvm-18/llvm/IR/Type.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <llvm/Support/raw_ostream.h>
#include <memory>

CodegenContext ctx("MyModule");

llvm::Value *IntegerNode::codegen(CodegenContext &ctx) {
  return llvm::ConstantInt::get(ctx.Builder->getInt32Ty(), val);
}

llvm::Value *FloatNode::codegen(CodegenContext &ctx) {
  return llvm::ConstantFP::get(ctx.Builder->getFloatTy(), val);
}

llvm::Value *StringNode::codegen(CodegenContext &ctx) {
  return ctx.Builder->CreateGlobalStringPtr(val);
}

llvm::Value *BooleanNode::codegen(CodegenContext &ctx) {
  return llvm::ConstantInt::get(ctx.Builder->getInt1Ty(), val);
}

llvm::Value *IdentifierNode::codegen(CodegenContext &cc) {
  if (cc.NamedValues.count(val))
    return cc.NamedValues[val]; // Found variable
  throw std::runtime_error("Unknown variable: " + val);
}

// llvm::Value *BinaryOperationNode::codegen(CodegenContext &cc) {
//   llvm::Value *L = left->codegen(cc);
//   llvm::Value *R = right->codegen(cc);

//   if (!L || !R)
//     return nullptr;

//   switch (op) {
//   case PLUSOP:
//     return cc.Builder->CreateFAdd(L, R, "addtmp");
//   case MINUSOP:
//     return cc.Builder->CreateFSub(L, R, "subtmp");
//   case MULOP:
//     return cc.Builder->CreateFMul(L, R, "multmp");
//   case DIVOP:
//     return cc.Builder->CreateFDiv(L, R, "divtmp");
//   case EQUALOP:
//     return cc.Builder->CreateFCmpUEQ(L, R, "eqtmp");
//   case NOTEQUALOP:
//     return cc.Builder->CreateFCmpUNE(L, R, "netmp");
//   case LESSTHANOP:
//     return cc.Builder->CreateFCmpULT(L, R, "lttmp");
//   case GREATERTHANOP:
//     return cc.Builder->CreateFCmpUGT(L, R, "gttmp");
//   case LESSTHANEQUALOP:
//     return cc.Builder->CreateFCmpULE(L, R, "letmp");
//   case GREATERTHANEQUALOP:
//     return cc.Builder->CreateFCmpUGE(L, R, "getmp");
//   case ANDOP:
//     return cc.Builder->CreateAnd(L, R, "andtmp");
//   case OROP:
//     return cc.Builder->CreateOr(L, R, "ortmp");
//   default:
//     throw std::runtime_error("Unknown binary operator");
//     return nullptr;
//   }
// }

llvm::Value *BinaryOperationNode::codegen(CodegenContext &cc) {
  llvm::Value *L = left->codegen(cc);
  llvm::Value *R = right->codegen(cc);
  if (!L || !R)
    return nullptr;

  switch (op) {
  case PLUSOP:
    return cc.Builder->CreateFAdd(L, R, "addtmp");
  case MINUSOP:
    return cc.Builder->CreateFSub(L, R, "subtmp");
  case MULOP:
    return cc.Builder->CreateFMul(L, R, "multmp");
  case DIVOP:
    return cc.Builder->CreateFDiv(L, R, "divtmp");

  case EQUALOP:
    return cc.Builder->CreateFCmpOEQ(L, R, "eqtmp");
  case NOTEQUALOP:
    return cc.Builder->CreateFCmpONE(L, R, "netmp");
  case LESSTHANOP:
    return cc.Builder->CreateFCmpOLT(L, R, "lttmp");
  case GREATERTHANOP:
    return cc.Builder->CreateFCmpOGT(L, R, "gttmp");
  case LESSTHANEQUALOP:
    return cc.Builder->CreateFCmpOLE(L, R, "letmp");
  case GREATERTHANEQUALOP:
    return cc.Builder->CreateFCmpOGE(L, R, "getmp");

  case ANDOP: {
    auto *zero = llvm::ConstantFP::get(L->getType(), 0.0);
    L = cc.Builder->CreateFCmpONE(L, zero, "lbool");
    R = cc.Builder->CreateFCmpONE(R, zero, "rbool");
    return cc.Builder->CreateAnd(L, R, "andtmp");
  }

  case OROP: {
    auto *zero = llvm::ConstantFP::get(L->getType(), 0.0);
    L = cc.Builder->CreateFCmpONE(L, zero, "lbool");
    R = cc.Builder->CreateFCmpONE(R, zero, "rbool");
    return cc.Builder->CreateOr(L, R, "ortmp");
  }

  default:
    return nullptr;
  }
}

llvm::Value *VariableDeclareNode::codegen(CodegenContext &cc) {
  llvm::Type *llvmType = nullptr;

  switch (type) {
  case INTEGERTYPE:
    llvmType = llvm::Type::getInt32Ty(*cc.TheContext);
    break;
  case FLOATTYPE:
    llvmType = llvm::Type::getFloatTy(*cc.TheContext);
    break;
  case BOOLEANTYPE:
    llvmType = llvm::Type::getInt1Ty(*cc.TheContext);
    break;
  case STRINGTYPE:
    llvmType = llvm::PointerType::get(llvm::Type::getInt8Ty(*cc.TheContext), 0);
    break;
  default:
    throw std::runtime_error("Unknown type in variable declaration");
  }

  llvm::AllocaInst *alloca = cc.Builder->CreateAlloca(llvmType, 0, name);

  if (Content) {
    llvm::Value *initVal = Content->codegen(cc);
    cc.Builder->CreateStore(initVal, alloca);
  }

  cc.NamedValues[name] = alloca;
  return alloca;
}

llvm::Value *AssignmentNode::codegen(CodegenContext &cc) {
  auto varIt = cc.NamedValues.find(name);
  if (varIt == cc.NamedValues.end())
    throw std::runtime_error("Undefined variable: " + name);

  llvm::Value *val = Content->codegen(cc);
  cc.Builder->CreateStore(val, varIt->second);
  return val;
}

// llvm::Value *FunctionNode::codegen(CodegenContext &cc) {
//   llvm::FunctionType *funcType =
//       llvm::FunctionType::get(llvm::Type::getVoidTy(*cc.TheContext), false);

//   llvm::Function *func = llvm::Function::Create(
//       funcType, llvm::Function::ExternalLinkage, name, cc.Module.get());

//   llvm::BasicBlock *entry =
//       llvm::BasicBlock::Create(*cc.TheContext, "entry", func);
//   cc.Builder->SetInsertPoint(entry);

//   if (contents)
//     contents->codegen(cc);

//   cc.Builder->CreateRetVoid();
//   return func;
// }

llvm::Value *FunctionNode::codegen(CodegenContext &cc) {
  llvm::Type *retType = nullptr;

  if (ReturnType == Types::INTEGERTYPE) {
    retType = llvm::Type::getInt32Ty(*cc.TheContext);
  } else if (ReturnType == Types::BOOLEANTYPE) {
    retType = llvm::Type::getInt1Ty(*cc.TheContext);
  } else if (ReturnType == Types::STRINGTYPE) {
    retType = llvm::PointerType::get(llvm::Type::getInt8Ty(*cc.TheContext), 0);
  } else if (ReturnType == Types::FLOATTYPE) {
    retType = llvm::Type::getFloatTy(*cc.TheContext);
  } else if (ReturnType == Types::VOIDTYPE) {
    retType = llvm::Type::getVoidTy(*cc.TheContext);
  }

  std::vector<llvm::Type *> argTypes;
  for (auto &arg : args) {
    argTypes.push_back(arg->codegen(cc)->getType());
  }

  llvm::FunctionType *funcType =
      llvm::FunctionType::get(retType, argTypes, false);

  llvm::Function *func = llvm::Function::Create(
      funcType, llvm::Function::ExternalLinkage, name, cc.Module.get());

  // Name the arguments and store in NamedValues
  unsigned idx = 0;
  for (auto &arg : func->args()) {
    if (idx < args.size()) {
      arg.setName("arg" +
                  std::to_string(
                      idx)); // optional: get name from arg node if it has one
      cc.NamedValues[arg.getName().str()] = &arg;
    }
    ++idx;
  }

  // Create entry basic block
  llvm::BasicBlock *BB =
      llvm::BasicBlock::Create(*cc.TheContext, "entry", func);
  cc.Builder->SetInsertPoint(BB);

  // Generate body
  if (contents) {
    llvm::Value *retVal = contents->codegen(cc);
    // If function is not void and body doesn't generate a return, add one
    if (!retType->isVoidTy() && retVal) {
      cc.Builder->CreateRet(retVal);
    } else if (retType->isVoidTy()) {
      cc.Builder->CreateRetVoid();
    }
  } else {
    // No body
    if (retType->isVoidTy()) {
      cc.Builder->CreateRetVoid();
    }
  }

  return func;
}

llvm::Value *CompountNode::codegen(CodegenContext &cc) {
  llvm::Value *last = nullptr;
  for (auto &stmt : contents) {
    last = stmt->codegen(cc);
  }
  return last; // last statement value
}

llvm::Value *IfNode::codegen(CodegenContext &cc) {
  llvm::Value *condVal = args->codegen(cc);
  if (!condVal)
    throw std::runtime_error("If condition codegen failed");

  // convert condition to i1 if needed
  if (!condVal->getType()->isIntegerTy(1)) {
    if (condVal->getType()->isIntegerTy())
      condVal = cc.Builder->CreateICmpNE(
          condVal, llvm::ConstantInt::get(condVal->getType(), 0), "tobool");
    else if (condVal->getType()->isFloatingPointTy())
      condVal = cc.Builder->CreateFCmpONE(
          condVal, llvm::ConstantFP::get(condVal->getType(), 0.0), "tobool");
    else if (condVal->getType()->isPointerTy())
      condVal = cc.Builder->CreateICmpNE(
          cc.Builder->CreatePtrToInt(condVal,
                                     llvm::Type::getInt64Ty(*cc.TheContext)),
          llvm::ConstantInt::get(llvm::Type::getInt64Ty(*cc.TheContext), 0),
          "ptrtobool");
    else
      throw std::runtime_error("Unsupported if condition type");
  }

  llvm::Function *func = cc.Builder->GetInsertBlock()->getParent();

  llvm::BasicBlock *thenBB =
      llvm::BasicBlock::Create(*cc.TheContext, "then", func);
  llvm::BasicBlock *elseBB =
      elseBlock ? llvm::BasicBlock::Create(*cc.TheContext, "else", func)
                : nullptr;
  llvm::BasicBlock *mergeBB =
      llvm::BasicBlock::Create(*cc.TheContext, "ifcont", func);

  if (elseBB)
    cc.Builder->CreateCondBr(condVal, thenBB, elseBB);
  else
    cc.Builder->CreateCondBr(condVal, thenBB, mergeBB);

  cc.Builder->SetInsertPoint(thenBB);
  if (block)
    block->codegen(cc);
  cc.Builder->CreateBr(mergeBB);

  if (elseBB) {
    cc.Builder->SetInsertPoint(elseBB);
    elseBlock->codegen(cc);
    cc.Builder->CreateBr(mergeBB);
  }

  cc.Builder->SetInsertPoint(mergeBB);
  return nullptr;
}

llvm::Value *WhileNode::codegen(CodegenContext &cc) {
  llvm::Function *func = cc.Builder->GetInsertBlock()->getParent();

  llvm::BasicBlock *condBB =
      llvm::BasicBlock::Create(*cc.TheContext, "whilecond", func);
  llvm::BasicBlock *bodyBB =
      llvm::BasicBlock::Create(*cc.TheContext, "whilebody", func);
  llvm::BasicBlock *afterBB =
      llvm::BasicBlock::Create(*cc.TheContext, "afterwhile", func);

  cc.Builder->CreateBr(condBB);

  cc.Builder->SetInsertPoint(condBB);
  llvm::Value *condVal = args->codegen(cc);
  if (!condVal)
    throw std::runtime_error("While condition codegen failed");

  // convert condition to i1
  if (!condVal->getType()->isIntegerTy(1)) {
    if (condVal->getType()->isIntegerTy())
      condVal = cc.Builder->CreateICmpNE(
          condVal, llvm::ConstantInt::get(condVal->getType(), 0), "tobool");
    else if (condVal->getType()->isFloatingPointTy())
      condVal = cc.Builder->CreateFCmpONE(
          condVal, llvm::ConstantFP::get(condVal->getType(), 0.0), "tobool");
    else if (condVal->getType()->isPointerTy())
      condVal = cc.Builder->CreateICmpNE(
          cc.Builder->CreatePtrToInt(condVal,
                                     llvm::Type::getInt64Ty(*cc.TheContext)),
          llvm::ConstantInt::get(llvm::Type::getInt64Ty(*cc.TheContext), 0),
          "ptrtobool");
    else
      throw std::runtime_error("Unsupported while condition type");
  }

  cc.Builder->CreateCondBr(condVal, bodyBB, afterBB);

  cc.Builder->SetInsertPoint(bodyBB);
  if (block)
    block->codegen(cc);
  cc.Builder->CreateBr(condBB);

  cc.Builder->SetInsertPoint(afterBB);
  return nullptr;
}

int main() {
  CodegenContext ctx("My_Module");

  // 40 + 2
  auto lhs = std::make_unique<IntegerNode>(40);
  auto rhs = std::make_unique<IntegerNode>(2);

  auto bin = std::make_unique<BinaryOperationNode>(
      std::move(lhs), std::move(rhs), BinaryOpTokentype::PLUSOP);

  std::vector<std::unique_ptr<ast>> noArgs;

  auto fn =
      std::make_unique<FunctionNode>("test",
                                     std::move(bin), // function body
                                     Types::INTEGERTYPE, std::move(noArgs));

  fn->codegen(ctx);
  ctx.Module->print(llvm::errs(), nullptr);
}
