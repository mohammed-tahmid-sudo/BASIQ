#include <ast.h>
#include <llvm-18/llvm/IR/Constants.h>
#include <llvm-18/llvm/IR/Type.h>
#include <llvm/IR/Value.h>

CodegenContext ctx("MyModule");

llvm::Value *IntLiteralNode::codegen(CodegenContext &ctx) {
  return llvm::ConstantInt::get(ctx.Builder->getInt32Ty(), val);
}

llvm::Value *FloatLiteralNode::codegen(CodegenContext &ctx) {
  return llvm::ConstantFP::get(ctx.Builder->getFloatTy(), val);
}

llvm::Value *StringLiteralNode::codegen(CodegenContext &ctx) {
  return ctx.Builder->CreateGlobalStringPtr(val);
}

llvm::Value *BooleanLiteralNode::codegen(CodegenContext &ctx) {
  return llvm::ConstantInt::get(ctx.Builder->getInt1Ty(), val);
}
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
    return cc.Builder->CreateFCmpUEQ(L, R, "eqtmp");
  case NOTEQUALOP:
    return cc.Builder->CreateFCmpUNE(L, R, "netmp");
  case LESSTHANOP:
    return cc.Builder->CreateFCmpULT(L, R, "lttmp");
  case GREATERTHANOP:
    return cc.Builder->CreateFCmpUGT(L, R, "gttmp");
  case LESSTHANEQUALOP:
    return cc.Builder->CreateFCmpULE(L, R, "letmp");
  case GREATERTHANEQUALOP:
    return cc.Builder->CreateFCmpUGE(L, R, "getmp");
  case ANDOP:
    return cc.Builder->CreateAnd(L, R, "andtmp");
  case OROP:
    return cc.Builder->CreateOr(L, R, "ortmp");
  default:
    throw std::runtime_error("Unknown binary operator");
  }
}

llvm::Value *VariableDeclareNode(CodegenContext &cc) {
  llvm::Type *llvmType = nullptr;

  switch (type) {
  case Types::INTEGERTYPE:
    llvmType = llvm::Type::getInt32Ty(*ctx.TheContext);
    break;
  }
}
