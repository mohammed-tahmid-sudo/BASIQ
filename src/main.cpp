#include "codegen.h"
#include "globals.h"
#include <ast.h>
#include <llvm-18/llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>

int main() {
    TheContext = std::make_unique<llvm::LLVMContext>();
    TheModule = std::make_unique<llvm::Module>("my_module", *TheContext);
    Builder = std::make_unique<llvm::IRBuilder<>>(*TheContext);

    auto *FT = llvm::FunctionType::get(llvm::Type::getDoubleTy(*TheContext), false);
    auto *F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, "main", TheModule.get());
    auto *BB = llvm::BasicBlock::Create(*TheContext, "entry", F);
    Builder->SetInsertPoint(BB);

    auto code = std::make_unique<BinaryOperationNode>(
        std::make_unique<NumberNode>(23),
        std::make_unique<NumberNode>(33),
        '+'
    );

    llvm::Value* Result = code->codegen();
    Builder->CreateRet(Result);

    llvm::verifyFunction(*F, &llvm::errs());
    TheModule->print(llvm::outs(), nullptr);
}

