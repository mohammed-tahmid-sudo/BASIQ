#include <llvm-18/llvm/IR/IRBuilder.h>
#include <llvm-18/llvm/IR/LLVMContext.h>
#include <llvm-18/llvm/IR/Value.h>
#include <string>
#include <iostream>

static llvm::LLVMContext TheContext;
static std::unique_ptr<llvm::Module> TheModule =
    std::make_unique<llvm::Module>("my_module", TheContext);
static llvm::IRBuilder<> Builder(TheContext);
static std::unordered_map<std::string, llvm::AllocaInst *>
    NamedValues; // variable name -> alloca (or value)

struct ast
{
  virtual std::string repr() = 0;
  virtual llvm::Value *codegen(llvm::LLVMContext &context,
                               llvm::IRBuilder<> &builder) = 0;
  virtual ~ast() = 0;
};

class NumberNode : public ast
{
public:
  int number;

  llvm::Value *codegen(llvm::LLVMContext &context,
                       llvm::IRBuilder<> &builder)
  {
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), number, true);
  }
  std::string repr()
  {
    return "NumberNode(" + number + ')';
  }
};

class VariableNode : public ast
{
public:
  std::string VarName;
  std::string Type;

  llvm::Value *codegen(llvm::LLVMContext &context, llvm::IRBuilder<> &builder) override
  {
    auto it = NamedValues.find(VarName);
    if (it == NamedValues.end())
      throw std::runtime_error("Unknown variable: " + VarName);

    llvm::AllocaInst *alloca = it->second;

    // LLVM 18 requires the type explicitly
    return builder.CreateLoad(alloca->getAllocatedType(), alloca, VarName + "_val");
  }
};


llvm::Function *createMainFunction() {
    llvm::FunctionType *FT =
        llvm::FunctionType::get(llvm::Type::getInt32Ty(TheContext), false);
    llvm::Function *F =
        llvm::Function::Create(FT, llvm::Function::ExternalLinkage, "main", TheModule.get());

    llvm::BasicBlock *BB = llvm::BasicBlock::Create(TheContext, "entry", F);
    Builder.SetInsertPoint(BB);

    return F;
}

llvm::AllocaInst *createEntryBlockAlloca(llvm::Function *TheFunction, const std::string &VarName) {
    llvm::IRBuilder<> TmpB(&TheFunction->getEntryBlock(),
                           TheFunction->getEntryBlock().begin());
    return TmpB.CreateAlloca(llvm::Type::getInt32Ty(TheContext), nullptr, VarName);
}



// ===== Helpers =====
llvm::Function *createMainFunction() {
    auto FT = llvm::FunctionType::get(llvm::Type::getInt32Ty(TheContext), false);
    auto F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, "main", TheModule.get());
    auto BB = llvm::BasicBlock::Create(TheContext, "entry", F);
    Builder.SetInsertPoint(BB);
    return F;
}

llvm::AllocaInst *createEntryBlockAlloca(llvm::Function *TheFunction, const std::string &VarName) {
    llvm::IRBuilder<> TmpB(&TheFunction->getEntryBlock(), TheFunction->getEntryBlock().begin());
    return TmpB.CreateAlloca(llvm::Type::getInt32Ty(TheContext), nullptr, VarName);
}

// ===== Main =====
int main() {
    // Use unique_ptr for AST nodes
    std::unique_ptr<NumberNode> numNode = std::make_unique<NumberNode>();
    numNode->number = 42;

    std::unique_ptr<VariableNode> varNode = std::make_unique<VariableNode>();
    varNode->VarName = "x";

    // Function and variable
    llvm::Function *MainFunc = createMainFunction();
    llvm::AllocaInst *xAlloca = createEntryBlockAlloca(MainFunc, "x");
    NamedValues["x"] = xAlloca;

    // Store and load
    Builder.CreateStore(numNode->codegen(TheContext, Builder), xAlloca);
    llvm::Value *loadedVal = varNode->codegen(TheContext, Builder);

    // Return
    Builder.CreateRet(loadedVal);

    // Print IR
    TheModule->print(llvm::outs(), nullptr);
}