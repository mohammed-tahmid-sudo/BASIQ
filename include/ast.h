#pragma once
#include "llvm/IR/Value.h"
#include <iostream>
#include <map>
#include <memory> // for std::unique_ptr
#include <string>
#include <vector>

inline llvm::Value *LogErrorV(const char *Str) {
  std::cerr << "LogError: " << Str << std::endl; // print to console
  return nullptr;
}

// -------- Base --------

struct ast {
  virtual ~ast() = default;

  virtual std::string repr() = 0;
  virtual llvm::Value *codegen() = 0;
};

// -------- Expression / Value --------

class NumberNode : public ast {
public:
  double number;
  explicit NumberNode(int n);
  std::string repr() override;
  llvm::Value *codegen() override;
};

class VariableNode : public ast {
public:
  std::string name;
  explicit VariableNode(const std::string &n);
  std::string repr() override;
  llvm::Value *codegen() override;
};

class VariableDeclareNode : public ast {
public:
  std::string name;
  std::string type;
  std::unique_ptr<ast> contents;

  VariableDeclareNode(std::string n, std::string tp, std::unique_ptr<ast>cntnt);

  std::string repr() override;
  llvm::Value *codegen() override;
};

class AssignmentNode : public ast {
public:
  std::unique_ptr<VariableNode> name;
  std::unique_ptr<ast> value;
  std::string type;

  AssignmentNode(std::unique_ptr<VariableNode> n,
                 std::unique_ptr<ast> v,
                 const std::string &t = "");
  std::string repr() override;
  llvm::Value *codegen() override;
};

class BinaryOperationNode : public ast {
public:
  std::unique_ptr<ast> left;
  std::unique_ptr<ast> right;
  char op;

  BinaryOperationNode(std::unique_ptr<ast> l, std::unique_ptr<ast> r,
                      const char o);
  std::string repr() override;
  llvm::Value *codegen() override;
};

class IdentifierNode : public ast {
public:
  std::string id;
  explicit IdentifierNode(const std::string &n);
  std::string repr() override;
  llvm::Value *codegen() override;
};

class StringNode : public ast {
public:
  std::string value;
  explicit StringNode(const std::string &v);
  std::string repr() override;
  llvm::Value *codegen() override;
};

// -------- Control Flow --------

class ReturnNode : public ast {
public:
  std::unique_ptr<ast> expr;
  explicit ReturnNode(std::unique_ptr<ast> e = nullptr);
  std::string repr() override;
  llvm::Value *codegen() override;
};

class BreakNode : public ast {
public:
  std::string repr() override;
  llvm::Value *codegen() override;
};

class ContinueNode : public ast {
public:
  std::string repr() override;
  llvm::Value *codegen() override;
};

class ComparisonNode : public ast {
public:
  std::vector<std::unique_ptr<ast>> left;
  std::vector<std::unique_ptr<ast>> right;
  std::string comp;

  ComparisonNode(std::vector<std::unique_ptr<ast>> l,
                 std::vector<std::unique_ptr<ast>> r, const std::string &c);
  std::string repr() override;
  llvm::Value *codegen() override;
};

class IfNode : public ast {
public:
  std::vector<std::unique_ptr<ast>> condition;
  std::vector<std::unique_ptr<ast>> body;
  std::vector<std::unique_ptr<ast>> elseBody;

  IfNode(std::vector<std::unique_ptr<ast>> cond,
         std::vector<std::unique_ptr<ast>> ifBody,
         std::vector<std::unique_ptr<ast>> elseBody = {});
  std::string repr() override;
  llvm::Value *codegen() override;
};

class WhileNode : public ast {
public:
  std::unique_ptr<ast> condition;
  std::vector<std::unique_ptr<ast>> body;

  WhileNode(std::unique_ptr<ast> cond, std::vector<std::unique_ptr<ast>> b);
  std::string repr() override;
  llvm::Value *codegen() override;
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
  llvm::Value *codegen() override;
};

class FunctionNode : public ast {
public:
  std::string name;
  std::vector<std::string> parameters;
  std::vector<std::unique_ptr<ast>> body;

  FunctionNode(const std::string &n, std::vector<std::string> params,
               std::vector<std::unique_ptr<ast>> b);
  std::string repr() override;
  llvm::Value *codegen() override;
};

class PrintNode : public ast {
public:
  std::unique_ptr<ast> args;
  explicit PrintNode(std::unique_ptr<ast> arg);
  std::string repr() override;
  llvm::Value *codegen() override;
};
