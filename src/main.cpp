#include <ast.h>
#include <codegen.h>
#include <iostream>
#include <lexer.h>
#include <parser.h>

#include <llvm-18/llvm/ADT/APFloat.h>
#include <llvm-18/llvm/CodeGen/TargetPassConfig.h>
#include <llvm-18/llvm/IR/Constants.h>
#include <llvm-18/llvm/IR/DataLayout.h>
#include <llvm-18/llvm/IR/Function.h>
#include <llvm-18/llvm/IR/IRBuilder.h>
#include <llvm-18/llvm/IR/Instructions.h>
#include <llvm-18/llvm/IR/Intrinsics.h>
#include <llvm-18/llvm/IR/LegacyPassManager.h>
#include <llvm-18/llvm/IR/Type.h>
#include <llvm-18/llvm/IR/Value.h>
#include <llvm-18/llvm/IR/Verifier.h>
#include <llvm-18/llvm/Support/FileSystem.h>
#include <llvm-18/llvm/Support/Format.h>
#include <llvm-18/llvm/Support/TargetSelect.h>
#include <llvm-18/llvm/Support/raw_ostream.h>
#include <llvm-18/llvm/Target/TargetMachine.h>
#include <llvm-18/llvm/TargetParser/Host.h>
#include <llvm/MC/TargetRegistry.h>

#include <fstream>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

int main(int argc, char *argv[]) {
  if (argc < 2)
    return 1;

  std::ifstream file(argv[1]);
  Lexer lex;
  std::vector<std::vector<Token>> output_lexed;
  std::string line;

  while (std::getline(file, line)) {
    std::cout << line << "\n";
    auto lineTokens = lex.lexerSplitStatements(line);
    output_lexed.insert(output_lexed.end(), lineTokens.begin(),
                        lineTokens.end());
  }

  for (auto &tok : output_lexed) {
    for (auto &tk : tok) {
      std::cout << tokenTypeToString(tk.Type) << " : " << tk.Value << std::endl;
    }
  }

  Parser parse(output_lexed);

  std::vector<std::unique_ptr<ast>> code = parse.Parse();

  for (auto &tok : code) {
    std::cout << tok->repr() << std::endl;
  }

  // Initialize LLVM target
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();

  // Context, Module, Builder
  auto TheContext = std::make_unique<llvm::LLVMContext>();
  auto TheModule = std::make_unique<llvm::Module>("my_module", *TheContext);
  auto Builder = std::make_unique<llvm::IRBuilder<>>(*TheContext);
  std::map<std::string, llvm::Value *> NamedValues;

  // Create main function
  auto *FT =
      llvm::FunctionType::get(llvm::Type::getInt32Ty(*TheContext), false);
  auto *F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, "main",
                                   TheModule.get());
  auto *BB = llvm::BasicBlock::Create(*TheContext, "entry", F);
  Builder->SetInsertPoint(BB);

  // Sample code
  // auto code =
  // std::make_unique<PrintNode>(std::make_unique<BinaryOperationNode>(
  //     std::make_unique<NumberNode>(5), std::make_unique<NumberNode>(5),
  //     '+'));

  // Codegen context
  CodegenContext cg{TheContext, Builder, TheModule, NamedValues};

  llvm::Value *Last = nullptr;

  for (auto &stmt : code)
    Last = stmt->codegen(cg);

  if (!Last || !Last->getType()->isIntegerTy(32))
    Last = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), 0);

  Builder->CreateRet(Last);

  TheModule->print(llvm::errs(), nullptr);

  // llvm::Value *Result = code->codegen(cg);
  // Builder->CreateRet(Result);

  if (llvm::verifyFunction(*F, &llvm::errs())) {
    llvm::errs() << "Function verification failed\n";
    return 1;
  }

  // --- AOT Compilation ---
  std::string TargetTriple = llvm::sys::getDefaultTargetTriple();
  TheModule->setTargetTriple(TargetTriple);

  std::string Error;
  const llvm::Target *Target =
      llvm::TargetRegistry::lookupTarget(TargetTriple, Error);
  if (!Target) {
    llvm::errs() << Error << "\n";
    return 1;
  }

  llvm::TargetOptions Options;
  std::optional<llvm::Reloc::Model> RM;
  auto TM =
      Target->createTargetMachine(TargetTriple, "generic", "", Options, RM);

  TheModule->setDataLayout(TM->createDataLayout());

  // Emit object file
  std::error_code EC;
  llvm::raw_fd_ostream dest("output.o", EC, llvm::sys::fs::OF_None);
  if (EC) {
    llvm::errs() << "Could not open file: " << EC.message() << "\n";
    return 1;
  }

  llvm::legacy::PassManager pass;
  if (TM->addPassesToEmitFile(pass, dest, nullptr,
                              llvm::CodeGenFileType::ObjectFile)) {
    llvm::errs() << "TargetMachine can't emit a file of this type\n";
    return 1;
  }

  pass.run(*TheModule);
  dest.close();

  // Link object to executable
  system("clang output.o -no-pie -o my_program");

  return 0;
}
