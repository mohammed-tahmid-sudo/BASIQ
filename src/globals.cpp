#include "globals.h"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>

std::unique_ptr<llvm::LLVMContext> TheContext = std::make_unique<llvm::LLVMContext>();
std::unique_ptr<llvm::IRBuilder<>> Builder = std::make_unique<llvm::IRBuilder<>>(*TheContext);
std::unique_ptr<llvm::Module> TheModule = std::make_unique<llvm::Module>("BASIQ", *TheContext);
std::map<std::string, llvm::Value*> NamedValues;

