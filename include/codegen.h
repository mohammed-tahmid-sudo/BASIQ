#pragma once
#include <ast.h>
#include <llvm-18/llvm/IR/IRBuilder.h>
#include <llvm-18/llvm/IR/LLVMContext.h>
#include <llvm-18/llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <map>
#include <memory>
#include <string>

// Error helper
llvm::Value *LogErrorV(const char *Str);

static std::unique_ptr<llvm::LLVMContext> TheContext;
static std::unique_ptr<llvm::IRBuilder<>> Builder;
static std::unique_ptr<llvm::Module> TheModule;
static std::map<std::string, llvm::Value *> NamedValues;

// actual definition and memory allocation
llvm::BasicBlock* CurrentLoopStart = nullptr;
llvm::BasicBlock* CurrentLoopEnd = nullptr;

// just declare it, no memory allocated yet
extern llvm::BasicBlock* CurrentLoopStart;
extern llvm::BasicBlock* CurrentLoopEnd;

