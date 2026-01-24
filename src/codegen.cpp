#include <alloca.h>
#include <ast.h>
#include <codegen.h>
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
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/TargetSelect.h> // This contains InitializeNativeTarget* functions
#include <memory>

llvm::Value *NumberNode::codegen(CodegenContext &cg) {

  return llvm::ConstantFP::get(llvm::Type::getDoubleTy(*cg.TheContext), number);
}

llvm::Value *VariableNode::codegen(CodegenContext &cg) {
  llvm::Value *v = cg.NamedValues[name];
  if (!v)
    LogErrorV("Unknow Variable Name");
  return v;
}

llvm::Value *BinaryOperationNode::codegen(CodegenContext &cg) {
  llvm::Value *LHS = left->codegen(cg);

  llvm::Value *RHS = right->codegen(cg);

  if (!LHS || !RHS)
    return nullptr;
  switch (op) {

  case '+':
    return cg.Builder->CreateFAdd(LHS, RHS, "addtmp");
  case '-':
    return cg.Builder->CreateFSub(LHS, RHS, "subtmp");
  case '*':
    return cg.Builder->CreateFMul(LHS, RHS, "multmp");
  case '<':
    LHS = cg.Builder->CreateFCmpULT(LHS, RHS, "cmptmp");
    // Convert bool 0/1 to double 0.0 or 1.0
    return cg.Builder->CreateUIToFP(
        LHS, llvm::Type::getDoubleTy(*cg.TheContext), "booltmp");
  default:
    return LogErrorV("invalid binary operator");
  }
}

llvm::Value *VariableDeclareNode::codegen(CodegenContext &cg) {
  llvm::Type *Type = nullptr;

  if (type == "STRING") {
    Type = llvm::Type::getInt8Ty(*cg.TheContext);
  } else if (type == "INTEGER") {
    Type = llvm::Type::getInt32Ty(*cg.TheContext);
  } else if (type == "FLOAT") {
    Type = llvm::Type::getFloatTy(*cg.TheContext);
  } else {
    LogErrorV("INVALID VARIABLE DELACRATION TYPE");
  }

  llvm::Function *func = cg.Builder->GetInsertBlock()->getParent();
  llvm::IRBuilder<> tmpbuilder(&func->getEntryBlock(),
                               func->getEntryBlock().begin());
  llvm::AllocaInst *alloca = tmpbuilder.CreateAlloca(Type, nullptr, name);

  // Initialize if there is an initializer
  if (contents) {
    llvm::Value *initVal = contents->codegen(cg);
    if (!initVal)
      return nullptr;
    cg.Builder->CreateStore(initVal, alloca);
  }

  cg.NamedValues[name] = alloca;
  return alloca;
}

llvm::Value *AssignmentNode::codegen(CodegenContext &cg) {
  llvm::Value *var = cg.NamedValues[name->name];
  if (!var) {
    std::cerr << "Unknown variable name: " << name->name << std::endl;
    return nullptr;
  }

  llvm::Value *val = value->codegen(cg);
  if (!val)
    return nullptr;

  cg.Builder->CreateStore(val, var);
  return val;
}

llvm::Value *IdentifierNode::codegen(CodegenContext &cg) {
  return nullptr;
} // DON"T NEED IT. I THINK...

llvm::Value *StringNode::codegen(CodegenContext &cg) {
  return cg.Builder->CreateGlobalStringPtr(value);
}

llvm::Value *ReturnNode::codegen(CodegenContext &cg) {
  llvm::Value *V = expr ? expr->codegen(cg) : nullptr;
  return cg.Builder->CreateRet(V);
}

llvm::Value *BreakNode::codegen(CodegenContext &cg) {
  return cg.Builder->CreateBr(CurrentLoopEnd);
}

llvm::Value *ContinueNode::codegen(CodegenContext &cg) {
  return cg.Builder->CreateBr(CurrentLoopStart);
}

llvm::Value *ComparisonNode::codegen(CodegenContext &cg) {

  llvm::Value *L = left->codegen(cg);
  llvm::Value *R = right->codegen(cg);

  if (!L || !R) {
    return nullptr;
  }

  llvm::Type *Type = nullptr;

  if (comp == "==")
    return cg.Builder->CreateICmpEQ(L, R, "cmptmp");
  else if (comp == "!=")
    return cg.Builder->CreateICmpNE(L, R, "cmptmp");
  else if (comp == "<")
    return cg.Builder->CreateICmpSLT(L, R, "cmptmp");
  else if (comp == "<=")
    return cg.Builder->CreateICmpSLE(L, R, "cmptmp");
  else if (comp == ">")
    return cg.Builder->CreateICmpSGT(L, R, "cmptmp");
  else if (comp == ">=")
    return cg.Builder->CreateICmpSGE(L, R, "cmptmp");

  return LogErrorV("Unknown comparison operator");
}

llvm::Value *IfNode::codegen(CodegenContext &cg) {
  llvm::Value *CondV = condition->codegen(cg);
  if (!CondV)
    return nullptr;

  CondV = cg.Builder->CreateICmpNE(
      CondV, llvm::ConstantInt::get(CondV->getType(), 0), "ifcond");

  llvm::Function *TheFunction = cg.Builder->GetInsertBlock()->getParent();

  llvm::BasicBlock *ThenBB =
      llvm::BasicBlock::Create(*cg.TheContext, "then", TheFunction);
  llvm::BasicBlock *MergeBB =
      llvm::BasicBlock::Create(*cg.TheContext, "ifcont", TheFunction);
  llvm::BasicBlock *ElseBB =
      !elseBody.empty()
          ? llvm::BasicBlock::Create(*cg.TheContext, "else", TheFunction)
          : nullptr;

  if (ElseBB)
    cg.Builder->CreateCondBr(CondV, ThenBB, ElseBB);
  else
    cg.Builder->CreateCondBr(CondV, ThenBB, MergeBB);

  cg.Builder->SetInsertPoint(ThenBB);
  for (auto &stmt : body)
    stmt->codegen(cg);
  cg.Builder->CreateBr(MergeBB);

  if (ElseBB) {
    cg.Builder->SetInsertPoint(ElseBB);
    for (auto &stmt : elseBody)
      stmt->codegen(cg);
    cg.Builder->CreateBr(MergeBB);
  }

  cg.Builder->SetInsertPoint(MergeBB);
  return nullptr;
}

llvm::Value *WhileNode::codegen(CodegenContext &cg) {
  llvm::Function *TheFunction = cg.Builder->GetInsertBlock()->getParent();

  llvm::BasicBlock *CondBB =
      llvm::BasicBlock::Create(*cg.TheContext, "whilecond", TheFunction);
  llvm::BasicBlock *LoopBB =
      llvm::BasicBlock::Create(*cg.TheContext, "loop", TheFunction);
  llvm::BasicBlock *AfterBB =
      llvm::BasicBlock::Create(*cg.TheContext, "afterloop", TheFunction);

  // branch to condition first
  cg.Builder->CreateBr(CondBB);

  // condition block
  cg.Builder->SetInsertPoint(CondBB);
  llvm::Value *CondV = condition->codegen(cg);
  if (!CondV)
    return nullptr;
  CondV = cg.Builder->CreateICmpNE(
      CondV, llvm::ConstantInt::get(CondV->getType(), 0), "whilecond");

  cg.Builder->CreateCondBr(CondV, LoopBB, AfterBB);

  // loop block
  cg.Builder->SetInsertPoint(LoopBB);
  llvm::BasicBlock *PrevLoopStart = CurrentLoopStart;
  llvm::BasicBlock *PrevLoopEnd = CurrentLoopEnd;
  CurrentLoopStart = CondBB;
  CurrentLoopEnd = AfterBB;

  for (auto &stmt : body)
    stmt->codegen(cg);

  cg.Builder->CreateBr(CondBB); // back to condition

  // restore previous loop context
  CurrentLoopStart = PrevLoopStart;
  CurrentLoopEnd = PrevLoopEnd;

  // after loop
  cg.Builder->SetInsertPoint(AfterBB);

  return nullptr;
}

llvm::Value *ForNode::codegen(CodegenContext &cg) {
  return nullptr;
} // GONNA IMPLEMENT ITLATERll
llvm::Value *FunctionNode::codegen(CodegenContext &cg) {
  return nullptr;
} // THIS TOO
  //
llvm::Value *PrintNode::codegen(CodegenContext &cg) {
  llvm::Value *Val = args->codegen(cg);
  if (!Val)
    return nullptr;

  // declare printf if not already
  llvm::Function *PrintF = cg.TheModule->getFunction("printf");
  if (!PrintF) {
    std::vector<llvm::Type *> printfArgs;
    printfArgs.push_back(cg.Builder->getInt8Ty());
    llvm::FunctionType *printfType =
        llvm::FunctionType::get(cg.Builder->getInt32Ty(), printfArgs, true);
    PrintF = llvm::Function::Create(printfType, llvm::Function::ExternalLinkage,
                                    "printf", cg.TheModule.get());
  }

  // create format string for int
  llvm::Value *formatStr = cg.Builder->CreateGlobalStringPtr("%f\n");
  return cg.Builder->CreateCall(PrintF, {formatStr, Val}, "printfCall");
}

// int main() {

//   llvm::InitializeNativeTarget();
//   llvm::InitializeNativeTargetAsmPrinter();

//   TheContext = std::make_unique<llvm::LLVMContext>();
//   TheModule  = std::make_unique<llvm::Module>("my_module", *TheContext);
//   Builder    = std::make_unique<llvm::IRBuilder<>>(*TheContext);

//   // double main()
//   auto *FT = llvm::FunctionType::get(
//       llvm::Type::getDoubleTy(*TheContext), false);

//   auto *F = llvm::Function::Create(
//       FT, llvm::Function::ExternalLinkage, "main", TheModule.get());

//   auto *BB = llvm::BasicBlock::Create(*TheContext, "entry", F);
//   Builder->SetInsertPoint(BB);

//   auto code = std::make_unique<BinaryOperationNode>(
//       std::make_unique<NumberNode>(5),
//       std::make_unique<NumberNode>(5),
//       '+');

//   llvm::Value *Result = code->codegen(llvm::LLVMContext &TheContext,
//   llvm::IRBuilder<> &Builder, llvm::Module *Module, std::map<std::string,
//   llvm::Value *> &NamedValues); Builder->CreateRet(Result);

//   if (llvm::verifyFunction(*F, &llvm::errs())) {
//     llvm::errs() << "Function verification failed\n";
//     return 1;
//   }

//   // Print IR
//   TheModule->print(llvm::outs(), nullptr);

//   return 0;
// }
