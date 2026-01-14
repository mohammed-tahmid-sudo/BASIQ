#pragma once
#include <memory>
#include <map>
#include <string>
#include <llvm-18/llvm/IR/LLVMContext.h>
#include <llvm-18/llvm/IR/IRBuilder.h>
#include <llvm-18/llvm/IR/Module.h>
#include <ast.h>

// Global LLVM objects
extern std::unique_ptr<llvm::LLVMContext> TheContext;
extern std::unique_ptr<llvm::IRBuilder<>> Builder;
extern std::unique_ptr<llvm::Module> TheModule;
extern std::map<std::string, llvm::Value*> NamedValues;

// Error helper
llvm::Value* LogErrorV(const char* Str);

