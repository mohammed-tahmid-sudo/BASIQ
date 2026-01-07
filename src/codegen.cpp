// #include <ast.h>
// #include <codegen.h>
// #include <iostream>
// #include <llvm-18/llvm/IR/Constants.h>
// #include <llvm-18/llvm/IR/IRBuilder.h>
// #include <llvm-18/llvm/IR/LLVMContext.h>
// #include <llvm-18/llvm/IR/Type.h>
// #include <llvm-18/llvm/IR/Value.h>
// #include <string>
// #include <unordered_map>

// std::unordered_map<std::string, llvm::Value *> NamedValue;

// llvm::Value *NumberNode::codegen(llvm::LLVMContext &context,
//                                  llvm::IRBuilder<> &builder) {

//   llvm::Type *llvmtype = nullptr;

//   llvm::Value *Varptr = NamedValue[value];

//   llvm::Type *llvmtype = nullptr;

//   if (type == "STRING") {
//     llvmtype = llvm::Type::getInt8PtrTy(context); // pointer to i8 for strings
//   } else if (type == "INT") {
//     llvmtype = llvm::Type::getInt32Ty(context);
//   } else if (type == "FLOAT") {
//     llvmtype = llvm::Type::getFloatTy(context);
//   } else {
//     std::cerr << "Unknown type: " << type << "\n";
//     return nullptr;
//   }

//   return builder.CreateLoad(llvmtype, Varptr, "load_", value);
// }

// llvm::Value *VariableNode::codegen(llvm::LLVMContext &context,
//                                    llvm::IRBuilder<> &builder) {
//   auto it = NamedValue.find(name);
//   if (it == NamedValue.end()) {
//     std::cerr << "Unknown variable: " << name << "\n";
//     return nullptr;
//   }

//   llvm::Type *llvmtype = nullptr;

//   if (type == "STRING") {
//     llvmtype = llvm::Type::getInt8Ty(context); // pointer to i8 for strings
//   } else if (type == "INT") {
//     llvmtype = llvm::Type::getInt32Ty(context);
//   } else if (type == "FLOAT") {
//     llvmtype = llvm::Type::getFloatTy(context);
//   } else {
//     std::cerr << "Unknown type: " << type << "\n";
//     return nullptr;
//   }
//   return NamedValue[name];
// }

// llvm::Value *VariableDeclareNode::codegen(llvm::LLVMContext &context,
//                                           llvm::IRBuilder<> &builder) {}
