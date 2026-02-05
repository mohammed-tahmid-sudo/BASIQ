#pragma once
#include "lexer.h"
#include <algorithm>
#include <filesystem>
#include <llvm-18/llvm/IR/IRBuilder.h>
#include <llvm-18/llvm/IR/LLVMContext.h>
#include <llvm-18/llvm/IR/Value.h>
#include <memory>
#include <string>
#include <vector>

struct CodegenContext {
  std::unique_ptr<llvm::LLVMContext> TheContext;
  std::unique_ptr<llvm::IRBuilder<>> Builder;
  std::unique_ptr<llvm::Module> Module;
  std::vector<std::unordered_map<std::string, llvm::Value *>> NamedValuesStack;

  // Enter a new scope
  void pushScope() { NamedValuesStack.push_back({}); }

  // Exit current scope
  void popScope() { NamedValuesStack.pop_back(); }

  // Add a variable to the current scope
  void addVariable(const std::string &name, llvm::Value *value) {
    NamedValuesStack.back()[name] = value;
  }

  // Lookup variable (from innermost to outermost scope)
  llvm::Value *lookup(const std::string &name) {
    for (auto it = NamedValuesStack.rbegin(); it != NamedValuesStack.rend();
         ++it) {
      if (it->count(name))
        return (*it)[name];
    }
    return nullptr; // not found
  }

  CodegenContext(const std::string &name)
      : TheContext(std::make_unique<llvm::LLVMContext>()),
        Builder(std::make_unique<llvm::IRBuilder<>>(*TheContext)),
        Module(std::make_unique<llvm::Module>(name, *TheContext)) {}
};

struct ast {
  virtual ~ast() = default;
  virtual std::string repr() = 0;
  virtual llvm::Value *codegen(CodegenContext &cc) = 0;
};

struct IntegerNode : ast {
  int val;
  IntegerNode(const int v) : val(v) {}
  std::string repr() override;
  llvm::Value *codegen(CodegenContext &cc) override;
};

struct FloatNode : ast {
  float val;
  FloatNode(float v) : val(v) {}
  std::string repr() override;
  llvm::Value *codegen(CodegenContext &cc) override;
};

struct BooleanNode : ast {
  bool val;
  BooleanNode(bool v) : val(v) {}
  std::string repr() override;
  llvm::Value *codegen(CodegenContext &cc) override;
};

struct StringNode : ast {
  std::string val;
  StringNode(const std::string &v) : val(v) {}
  std::string repr() override;
  llvm::Value *codegen(CodegenContext &cc) override;
};

struct VariableDeclareNode : ast {
  std::string name;
  TokenType Type;
  std::unique_ptr<ast> val;
  VariableDeclareNode(const std::string &n, std::unique_ptr<ast> v, TokenType t)
      : name(n), val(std::move(v)), Type(t) {}
  std::string repr() override;
  llvm::Value *codegen(CodegenContext &cc) override;
};

struct AssignmentNode : ast {
  std::string name;
  std::unique_ptr<ast> val;
  AssignmentNode(const std::string &n, std::unique_ptr<ast> v)
      : name(n), val(std::move(v)) {}
  std::string repr() override;
  llvm::Value *codegen(CodegenContext &cc) override;
};

struct CompoundNode : ast {
  std::vector<std::unique_ptr<ast>> blocks;
  CompoundNode(std::vector<std::unique_ptr<ast>> b) : blocks(std::move(b)) {}
  std::string repr() override;
  llvm::Value *codegen(CodegenContext &cc) override;
};

struct FunctionNode : ast {
  std::string name;
  std::unique_ptr<ast> content;
  TokenType ReturnType;

  FunctionNode(const std::string &s, std::unique_ptr<ast> cntnt,
               TokenType RetType)
      : name(s), content(std::move(cntnt)), ReturnType(RetType) {}

  std::string repr() override;
  llvm::Value *codegen(CodegenContext &cc) override;
};
