#include "codegen.h"

std::unique_ptr<llvm::LLVMContext> TheContext;
std::unique_ptr<llvm::Module> TheModule;
std::unique_ptr<llvm::IRBuilder<>> Builder;
std::unordered_map<std::string, llvm::Value *> NamedValues;
