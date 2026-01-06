// src/main.cpp
#include <iostream>
#include <memory>

#include "ast.cpp" // your AST implementation (NumberNode, PrintNode, etc.)
#include "codegen.h"

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"

#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/ExecutionEngine/Orc/ThreadSafeModule.h"

#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/Error.h" // for cantFail

int main() {
    // Initialize target for JIT
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    // Initialize global variables
    TheContext = std::make_unique<llvm::LLVMContext>();
    TheModule = std::make_unique<llvm::Module>("BASIQ JIT", *TheContext);
    Builder = std::make_unique<llvm::IRBuilder<>>(*TheContext);

    // Build `int main()` in the module
    llvm::FunctionType *mainType =
        llvm::FunctionType::get(Builder->getInt32Ty(), false);
    llvm::Function *mainFunc =
        llvm::Function::Create(mainType, llvm::Function::ExternalLinkage, "main", TheModule.get());

    llvm::BasicBlock *entry = llvm::BasicBlock::Create(*TheContext, "entry", mainFunc);
    Builder->SetInsertPoint(entry);

    // // Example AST usage: print(42)
    // auto num = std::make_unique<NumberNode>(42);
    // auto print = std::make_unique<PrintNode>(std::move(num));
    // if (!print->codegen()) {
    //     std::cerr << "print.codegen() failed\n";
    //     return 1;
    // }

    Builder->CreateRet(llvm::ConstantInt::get(*TheContext, llvm::APInt(32, 0)));

    // Verify module
    if (llvm::verifyModule(*TheModule, &llvm::errs())) {
        std::cerr << "Module verification failed\n";
        return 1;
    }

    // Create LLJIT (returns Expected<std::unique_ptr<LLJIT>>)
    auto JITOrErr = llvm::orc::LLJITBuilder().create();
    if (!JITOrErr) {
        llvm::logAllUnhandledErrors(JITOrErr.takeError(), llvm::errs(), "LLJIT create error: ");
        return 1;
    }
    auto JIT = std::move(*JITOrErr);

    // Move module + context into a ThreadSafeModule and add to JIT
    llvm::orc::ThreadSafeModule TSM(std::move(TheModule), std::move(TheContext));
    if (auto err = JIT->addIRModule(std::move(TSM))) {
        llvm::logAllUnhandledErrors(std::move(err), llvm::errs(), "addIRModule error: ");
        return 1;
    }

    // Look up the symbol "main" and get an ExecutorAddr.
    auto LookupRes = JIT->lookup("main");
    if (!LookupRes) {
        llvm::logAllUnhandledErrors(LookupRes.takeError(), llvm::errs(), "lookup error: ");
        return 1;
    }

    // Extract ExecutorAddr (no .getAddress()); convert to function pointer using toPtr<T>()
    auto ExecAddr = llvm::cantFail(std::move(LookupRes)); // ExecutorAddr
    auto mainPtr = ExecAddr.toPtr<int (*)()>();
    if (!mainPtr) {
        std::cerr << "Failed to convert JIT address to function pointer\n";
        return 1;
    }

    // Call the JITed main
    int ret = mainPtr();
    return ret;
}

