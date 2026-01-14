#include <ast.h>
#include <llvm-18/llvm/ADT/APFloat.h>
#include <llvm/IR/Verifier.h>
#include <llvm-18/llvm/IR/Constants.h>
#include <llvm-18/llvm/IR/Intrinsics.h>
#include <llvm-18/llvm/IR/Type.h>
#include <llvm-18/llvm/IR/Value.h>

extern std::unique_ptr<llvm::LLVMContext> TheContext;
extern std::unique_ptr<llvm::IRBuilder<>> Builder;
extern std::unique_ptr<llvm::Module> TheModule;
extern std::map<std::string, llvm::Value*> NamedValues;

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

llvm::Value *VariableDeclareNode::codegen() { return nullptr; }
llvm::Value *AssignmentNode::codegen() { return nullptr; }
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

//int main() {

//    // Initialize global LLVM objects
//    TheContext = std::make_unique<llvm::LLVMContext>();
//    TheModule = std::make_unique<llvm::Module>("my_module", *TheContext);
//    Builder = std::make_unique<llvm::IRBuilder<>>(*TheContext);

//    // Create function: double main()
//    auto *FT = llvm::FunctionType::get(llvm::Type::getDoubleTy(*TheContext), false);
//    auto *F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, "main", TheModule.get());
//    auto *BB = llvm::BasicBlock::Create(*TheContext, "entry", F);
//    Builder->SetInsertPoint(BB);

//    auto code = std::make_unique<BinaryOperationNode>(
//        std::make_unique<NumberNode>(23), std::make_unique<NumberNode>(33), '+');
//	//GETTING THE SEGFAULT HERE
//    llvm::Value *Result = code->codegen();
//	///////////////////////////////////

//    Builder->CreateRet(Result);
//    llvm::verifyFunction(*F, &llvm::errs());

//    TheModule->print(llvm::outs(), nullptr);
//}


