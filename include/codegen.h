#ifndef BASIQ_CODEGEN_H
#define BASIQ_CODEGEN_H

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"

#include <memory>
#include <string>
#include <unordered_map>

extern std::unique_ptr<llvm::LLVMContext> TheContext;
extern std::unique_ptr<llvm::Module> TheModule;
extern std::unique_ptr<llvm::IRBuilder<>> Builder;
extern std::unordered_map<std::string, llvm::Value *> NamedValues;

#endif // BASIQ_CODEGEN_H
