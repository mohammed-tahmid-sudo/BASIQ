#include <llvm/IR/Value.h>
#include <llvm/IR/IRBuilder.h>

#include <memory>

enum Types { INTEGER, FLOAT, BOOLEAN, STRING };

struct CodeGeneration {
	std::unique_ptr<llvm::LLVMContext> TheContext;
	std::unique_ptr<llvm::IRBuilder<>> Builder;
	
	
};
