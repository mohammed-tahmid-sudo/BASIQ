#include <ast.h>
#include <codegen.h>
#include <fstream>
#include <lexer.h>
#include <llvm-18/llvm/ADT/APFloat.h>
#include <llvm-18/llvm/IR/Constants.h>
#include <llvm-18/llvm/IR/DataLayout.h>
#include <llvm-18/llvm/IR/Function.h>
#include <llvm-18/llvm/IR/IRBuilder.h>
#include <llvm-18/llvm/IR/Instructions.h>
#include <llvm-18/llvm/IR/Intrinsics.h>
#include <llvm-18/llvm/IR/Type.h>
#include <llvm-18/llvm/IR/Value.h>
#include <llvm-18/llvm/Support/Format.h>
#include <llvm-18/llvm/Target/TargetMachine.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/TargetSelect.h> // This contains InitializeNativeTarget* functions
#include <parser.h>

// int main(int argc, char *argv[]) {
//   // Initialize LLVM
//   llvm::InitializeNativeTarget();
//   llvm::InitializeNativeTargetAsmPrinter();

//   // Create LLVM objects
//   auto TheContext = std::make_unique<llvm::LLVMContext>();
//   auto TheModule = std::make_unique<llvm::Module>("my_module", *TheContext);
//   auto Builder = std::make_unique<llvm::IRBuilder<>>(*TheContext);
//   std::map<std::string, llvm::Value *> NamedValues;

//   // Create main function: double main()
//   auto *FT =
//       llvm::FunctionType::get(llvm::Type::getDoubleTy(*TheContext), false);
//   auto *F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage,
//   "main",
//                                    TheModule.get());

//   auto *BB = llvm::BasicBlock::Create(*TheContext, "entry", F);
//   Builder->SetInsertPoint(BB);

//   // Example code to generate
//   auto code = std::make_unique<BinaryOperationNode>(
//       std::make_unique<NumberNode>(5), std::make_unique<NumberNode>(5), '+');

//   // Construct CodegenContext with **raw references**
//   CodegenContext cg{
//       TheContext, // LLVMContext&
//       Builder,    // IRBuilder&
//       TheModule,  // Module&
//       NamedValues // map&
//   };

//   llvm::Value *Result = code->codegen(cg);
//   Builder->CreateRet(Result);

//   // Verify function
//   if (llvm::verifyFunction(*F, &llvm::errs())) {
//     llvm::errs() << "Function verification failed\n";
//     return 1;
//   }

//   // Print IR
//   TheModule->print(llvm::outs(), nullptr);

//   return 0;
// }



int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <source_file>\n";
        return 1;
    }

    std::string filename = argv[1];

    // Read file
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Could not open file: " << filename << "\n";
        return 1;
    }
    std::string source((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();

    // Lexer + Parser
    Lexer lex;
    auto statements = lex.lexerSplitStatements(source);

    Parser parse(statements);
    auto output = parse.Parse();

    // Initialize LLVM
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();

    auto TheContext = std::make_unique<llvm::LLVMContext>();
    auto TheModule = std::make_unique<llvm::Module>("my_module", *TheContext);
    auto Builder = std::make_unique<llvm::IRBuilder<>>(*TheContext);
    std::map<std::string, llvm::Value *> NamedValues;

    // Create main function
    auto *FT = llvm::FunctionType::get(llvm::Type::getInt32Ty(*TheContext), false);
    auto *F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, "main", TheModule.get());
    auto *BB = llvm::BasicBlock::Create(*TheContext, "entry", F);
    Builder->SetInsertPoint(BB);

    // Codegen each parsed statement
    CodegenContext cg{TheContext, Builder, TheModule, NamedValues};
    llvm::Value* last = nullptr;
    for (auto &stmt : output) {
        last = stmt->codegen(cg);
    }

    // Return value from last statement
    Builder->CreateRet(last ? last : llvm::ConstantInt::get(*TheContext, llvm::APInt(32, 0)));

    if (llvm::verifyFunction(*F, &llvm::errs())) {
        llvm::errs() << "Function verification failed\n";
        return 1;
    }

    // --- AOT Compilation: generate object file and then binary ---
    auto TargetTriple = llvm::sys::getDefaultTargetTriple();
    TheModule->setTargetTriple(TargetTriple);

    std::string Error;
    auto Target = llvm::TargetRegistry::lookupTarget(TargetTriple, Error);
    if (!Target) {
        llvm::errs() << Error;
        return 1;
    }

    llvm::TargetOptions opt;
    auto RM = llvm::Optional<llvm::Reloc::Model>();
    auto TargetMachine = Target->createTargetMachine(TargetTriple, "generic", "", opt, RM);

    TheModule->setDataLayout(TargetMachine->createDataLayout());

    std::error_code EC;
    llvm::raw_fd_ostream dest("output.o", EC, llvm::sys::fs::OF_None);
    if (EC) {
        llvm::errs() << "Could not open file: " << EC.message();
        return 1;
    }

    llvm::legacy::PassManagerBase pass;
    if (TargetMachine->addPassesToEmitFile(pass, dest, nullptr, llvm::CGFT_ObjectFile)) {
        llvm::errs() << "TargetMachine can't emit a file of this type\n";
        return 1;
    }

    pass.run(*TheModule);
    dest.flush();

    std::cout << "Object file generated: output.o\n";
    std::cout << "Run: clang++ output.o -o output_binary\n";

    return 0;
}

