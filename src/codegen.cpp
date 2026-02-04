#include <ast.h>
#include <llvm-18/llvm/IR/Constants.h>
#include <llvm-18/llvm/IR/DerivedTypes.h>
#include <llvm-18/llvm/IR/Type.h>
#include <llvm-18/llvm/IR/Value.h>
#include <llvm-18/llvm/Support/raw_ostream.h>
#include <memory>

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

int main() {
  CodegenContext ctx("myprogram");

  // 1. i8* type
  llvm::Type *i8Ty = llvm::Type::getInt8Ty(*ctx.TheContext);
  llvm::Type *i8PtrTy = llvm::PointerType::get(i8Ty, 0);

  // 2. Create a function: i8* main()
  llvm::FunctionType *funcType = llvm::FunctionType::get(i8PtrTy, false);
  llvm::Function *mainFunc = llvm::Function::Create(
      funcType, llvm::Function::ExternalLinkage, "main", ctx.Module.get());

  // 3. Create a basic block inside the function
  llvm::BasicBlock *entry =
      llvm::BasicBlock::Create(*ctx.TheContext, "entry", mainFunc);
  ctx.Builder->SetInsertPoint(entry);

  // 4. Generate StringNode
  auto node = std::make_unique<StringNode>("hello world");
  llvm::Value *val = node->codegen(ctx);

  // 5. Return the string pointer
  ctx.Builder->CreateRet(val);

  // 6. Print the module
  ctx.Module->print(llvm::outs(), nullptr);

  return 0;
}
