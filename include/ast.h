#pragma once
#include <algorithm>
#include <memory>
#include <string>
#include <vector>

// -------- Base --------

struct ast {
  virtual std::string repr() = 0;
  virtual ~ast() = default;
};

// -------- Expression / Value --------

class NumberNode : public ast {
public:
  int number;
  explicit NumberNode(int n);
  std::string repr() override;
};

class VariableNode : public ast {
public:
  std::string name;
  explicit VariableNode(const std::string &n);
  std::string repr() override;
};

class VariableDeclareNode : public ast {
public:
  std::unique_ptr<VariableNode> name;
  std::string type;
  std::vector<std::unique_ptr<ast>> contents;

  VariableDeclareNode(std::unique_ptr<VariableNode> n, std::string tp,
                      std::vector<std::unique_ptr<ast>> cntnts);

  std::string repr() override;
};

class AssignmentNode : public ast {
public:
  std::unique_ptr<VariableNode> name;
  std::vector<std::unique_ptr<ast>> value;
  std::string type;

  AssignmentNode(std::unique_ptr<VariableNode> n,
                 std::vector<std::unique_ptr<ast>> v,
                 const std::string &t = "");
  std::string repr() override;
};

class BinaryOperationNode : public ast {
public:
  std::unique_ptr<ast> left;
  std::unique_ptr<ast> right;
  std::string op;

  BinaryOperationNode(std::unique_ptr<ast> l, std::unique_ptr<ast> r,
                      const std::string &o);
  std::string repr() override;
};

class IdentifierNode : public ast {
public:
  std::string id;
  explicit IdentifierNode(const std::string &n);
  std::string repr() override;
};

class StringNode : public ast {
public:
  std::string value;
  explicit StringNode(const std::string &v);
  std::string repr() override;
};

// -------- Control Flow --------

class ReturnNode : public ast {
public:
  std::unique_ptr<ast> expr;
  explicit ReturnNode(std::unique_ptr<ast> e = nullptr);
  std::string repr() override;
};

class BreakNode : public ast {
public:
  std::string repr() override;
};

class ContinueNode : public ast {
public:
  std::string repr() override;
};

class ComparisonNode : public ast {
public:
  std::vector<std::unique_ptr<ast>> left;
  std::vector<std::unique_ptr<ast>> right;
  std::string comp;

  ComparisonNode(std::vector<std::unique_ptr<ast>> l,
                 std::vector<std::unique_ptr<ast>> r, const std::string &c);
  std::string repr() override;
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
};

class WhileNode : public ast {
public:
  std::unique_ptr<ast> condition;
  std::vector<std::unique_ptr<ast>> body;

  WhileNode(std::unique_ptr<ast> cond, std::vector<std::unique_ptr<ast>> b);
  std::string repr() override;
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
};

class FunctionNode : public ast {
public:
  std::string name;
  std::vector<std::string> parameters;
  std::vector<std::unique_ptr<ast>> body;

  FunctionNode(const std::string &n, std::vector<std::string> params,
               std::vector<std::unique_ptr<ast>> b);
  std::string repr() override;
};

class PrintNode : public ast {
public:
  std::unique_ptr<ast> args;
  explicit PrintNode(std::unique_ptr<ast> arg);
  std::string repr() override;
};
