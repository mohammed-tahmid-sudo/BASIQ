#pragma once
#include <iostream>
#include <llvm-18/llvm/IR/Value.h>
#include <memory>
#include <string>
#include <vector>

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

inline llvm::Value *LogErrorV(const char *Str) {
  std::cerr << "LogError: " << Str << std::endl; // print to console
  return nullptr;
}

// -------- Base --------

struct ast {
  virtual ~ast() = default;

  virtual std::string repr() = 0;
  virtual llvm::Value *
  codegen(llvm::LLVMContext &context, llvm::IRBuilder<> &builder,
          llvm::Module *module,
          std::map<std::string, llvm::Value *> &namedValues) = 0;
};

// -------- Expression / Value --------

class NumberNode : public ast {
public:
  double number;
  explicit NumberNode(double n);
  std::string repr() override;
  llvm::Value *
  codegen(llvm::LLVMContext &context, llvm::IRBuilder<> &builder,
          llvm::Module *module,
          std::map<std::string, llvm::Value *> &namedValues) override;
};

class VariableNode : public ast {
public:
  std::string name;
  explicit VariableNode(const std::string &n);
  std::string repr() override;
  llvm::Value *
  codegen(llvm::LLVMContext &context, llvm::IRBuilder<> &builder,
          llvm::Module *module,
          std::map<std::string, llvm::Value *> &namedValues) override;
};

class VariableDeclareNode : public ast {
public:
  std::string name;
  std::string type;
  std::unique_ptr<ast> contents;

  VariableDeclareNode(std::string n, std::string tp,
                      std::unique_ptr<ast> cntnt);

  std::string repr() override;
  llvm::Value *
  codegen(llvm::LLVMContext &context, llvm::IRBuilder<> &builder,
          llvm::Module *module,
          std::map<std::string, llvm::Value *> &namedValues) override;
};

class AssignmentNode : public ast {
public:
  std::unique_ptr<VariableNode> name;
  std::unique_ptr<ast> value;
  std::string type;

  AssignmentNode(std::unique_ptr<VariableNode> n, std::unique_ptr<ast> v,
                 const std::string &t = "");
  std::string repr() override;
  llvm::Value *
  codegen(llvm::LLVMContext &context, llvm::IRBuilder<> &builder,
          llvm::Module *module,
          std::map<std::string, llvm::Value *> &namedValues) override;
};

class BinaryOperationNode : public ast {
public:
  std::unique_ptr<ast> left;
  std::unique_ptr<ast> right;
  char op;

  BinaryOperationNode(std::unique_ptr<ast> l, std::unique_ptr<ast> r,
                      const char o);
  std::string repr() override;
  llvm::Value *
  codegen(llvm::LLVMContext &context, llvm::IRBuilder<> &builder,
          llvm::Module *module,
          std::map<std::string, llvm::Value *> &namedValues) override;
};

class IdentifierNode : public ast {
public:
  std::string id;
  explicit IdentifierNode(const std::string &n);
  std::string repr() override;
  llvm::Value *
  codegen(llvm::LLVMContext &context, llvm::IRBuilder<> &builder,
          llvm::Module *module,
          std::map<std::string, llvm::Value *> &namedValues) override;
};

class StringNode : public ast {
public:
  std::string value;
  explicit StringNode(const std::string &v);
  std::string repr() override;
  llvm::Value *
  codegen(llvm::LLVMContext &context, llvm::IRBuilder<> &builder,
          llvm::Module *module,
          std::map<std::string, llvm::Value *> &namedValues) override;
};

// -------- Control Flow --------

class ReturnNode : public ast {
public:
  std::unique_ptr<ast> expr;
  explicit ReturnNode(std::unique_ptr<ast> e = nullptr);
  std::string repr() override;
  llvm::Value *
  codegen(llvm::LLVMContext &context, llvm::IRBuilder<> &builder,
          llvm::Module *module,
          std::map<std::string, llvm::Value *> &namedValues) override;
};

class BreakNode : public ast {
public:
  std::string repr() override;
  llvm::Value *
  codegen(llvm::LLVMContext &context, llvm::IRBuilder<> &builder,
          llvm::Module *module,
          std::map<std::string, llvm::Value *> &namedValues) override;
};

class ContinueNode : public ast {
public:
  std::string repr() override;
  llvm::Value *
  codegen(llvm::LLVMContext &context, llvm::IRBuilder<> &builder,
          llvm::Module *module,
          std::map<std::string, llvm::Value *> &namedValues) override;
};

class ComparisonNode : public ast {
public:
  std::unique_ptr<ast> left;
  std::unique_ptr<ast> right;
  std::string comp;

  ComparisonNode(std::unique_ptr<ast> l, std::unique_ptr<ast> r,
                 const std::string &c);
  std::string repr() override;
  llvm::Value *
  codegen(llvm::LLVMContext &context, llvm::IRBuilder<> &builder,
          llvm::Module *module,
          std::map<std::string, llvm::Value *> &namedValues) override;
};

class IfNode : public ast {
public:
  std::unique_ptr<ast> condition;
  std::vector<std::unique_ptr<ast>> body;
  std::vector<std::unique_ptr<ast>> elseBody;

  IfNode(std::unique_ptr<ast> cond, std::vector<std::unique_ptr<ast>> ifBody,
         std::vector<std::unique_ptr<ast>> elseBody = {});
  std::string repr() override;
  llvm::Value *
  codegen(llvm::LLVMContext &context, llvm::IRBuilder<> &builder,
          llvm::Module *module,
          std::map<std::string, llvm::Value *> &namedValues) override;
};

class WhileNode : public ast {
public:
  std::unique_ptr<ast> condition;
  std::vector<std::unique_ptr<ast>> body;

  WhileNode(std::unique_ptr<ast> cond, std::vector<std::unique_ptr<ast>> b);
  std::string repr() override;
  llvm::Value *
  codegen(llvm::LLVMContext &context, llvm::IRBuilder<> &builder,
          llvm::Module *module,
          std::map<std::string, llvm::Value *> &namedValues) override;
};

class ForNode : public ast {
public:
  std::unique_ptr<ast> init;
  std::unique_ptr<ast> condition;
  std::unique_ptr<ast> increment;
  std::vector<std::unique_ptr<ast>> body;

  ForNode(std::unique_ptr<ast> i, std::unique_ptr<ast> cond,
          std::unique_ptr<ast> inc, std::vector<std::unique_ptr<ast>> b);
  std::string repr() override;
  llvm::Value *
  codegen(llvm::LLVMContext &context, llvm::IRBuilder<> &builder,
          llvm::Module *module,
          std::map<std::string, llvm::Value *> &namedValues) override;
};

class FunctionNode : public ast {
public:
  std::string name;
  std::vector<std::string> parameters;
  std::vector<std::unique_ptr<ast>> body;

  FunctionNode(const std::string &n, std::vector<std::string> params,
               std::vector<std::unique_ptr<ast>> b);
  std::string repr() override;
  llvm::Value *
  codegen(llvm::LLVMContext &context, llvm::IRBuilder<> &builder,
          llvm::Module *module,
          std::map<std::string, llvm::Value *> &namedValues) override;
};

class PrintNode : public ast {
public:
  std::unique_ptr<ast> args;
  explicit PrintNode(std::unique_ptr<ast> arg);
  std::string repr() override;
  llvm::Value *
  codegen(llvm::LLVMContext &context, llvm::IRBuilder<> &builder,
          llvm::Module *module,
          std::map<std::string, llvm::Value *> &namedValues) override;
};
