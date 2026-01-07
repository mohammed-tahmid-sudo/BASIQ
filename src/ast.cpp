#include "ast.h"

// base
ast::~ast() = default;

// -------- Expressions --------
NumberNode::NumberNode(int n) : number(n) {}
std::string NumberNode::repr() {
  return "NumberNode(" + std::to_string(number) + ")";
}

VariableNode::VariableNode(const std::string &n, const std::string &t)
    : name(n), type(t) {}
std::string VariableNode::repr() { return "VariableNode(" + name + ")"; }

VariableDeclareNode::VariableDeclareNode(const std::string &n,
                                         const std::string &t)
    : name(n), type(t) {}
std::string VariableDeclareNode::repr() {
  return "VariableDeclareNode(" + name + ", " + type + ")";
}

AssignmentNode::AssignmentNode(std::unique_ptr<VariableNode> n,
                               std::vector<std::unique_ptr<ast>> v,
                               const std::string &t)
    : name(std::move(n)), value(std::move(v)), type(t) {}
std::string AssignmentNode::repr() { return "AssignmentNode"; }

BinaryOperationNode::BinaryOperationNode(std::unique_ptr<ast> l,
                                         std::unique_ptr<ast> r,
                                         const std::string &o)
    : left(std::move(l)), right(std::move(r)), op(o) {}
std::string BinaryOperationNode::repr() { return "BinaryOp(" + op + ")"; }

IdentifierNode::IdentifierNode(const std::string &n) : id(n) {}
std::string IdentifierNode::repr() { return "IdentifierNode(" + id + ")"; }

// -------- Control Flow --------
ReturnNode::ReturnNode(std::unique_ptr<ast> e) : expr(std::move(e)) {}
std::string ReturnNode::repr() { return "ReturnNode"; }

std::string BreakNode::repr() { return "BreakNode"; }
std::string ContinueNode::repr() { return "ContinueNode"; }

StringNode::StringNode(const std::string &v) : value(v) {}
std::string StringNode::repr() { return "StringNode(" + value + ")"; }

ComparisonNode::ComparisonNode(std::vector<std::unique_ptr<ast>> l,
                               std::vector<std::unique_ptr<ast>> r,
                               const std::string &c)
    : left(std::move(l)), right(std::move(r)), comp(c) {}
std::string ComparisonNode::repr() { return "ComparisonNode(" + comp + ")"; }

IfNode::IfNode(std::vector<std::unique_ptr<ast>> cond,
               std::vector<std::unique_ptr<ast>> ifBody,
               std::vector<std::unique_ptr<ast>> elseB)
    : condition(std::move(cond)), body(std::move(ifBody)),
      elseBody(std::move(elseB)) {}
std::string IfNode::repr() { return "IfNode"; }

WhileNode::WhileNode(std::unique_ptr<ast> cond,
                     std::vector<std::unique_ptr<ast>> b)
    : condition(std::move(cond)), body(std::move(b)) {}
std::string WhileNode::repr() { return "WhileNode"; }

ForNode::ForNode(std::unique_ptr<ast> i, std::unique_ptr<ast> cond,
                 std::unique_ptr<ast> inc, std::vector<std::unique_ptr<ast>> b)
    : init(std::move(i)), condition(std::move(cond)), increment(std::move(inc)),
      body(std::move(b)) {}
std::string ForNode::repr() { return "ForNode"; }

FunctionNode::FunctionNode(const std::string &n,
                           std::vector<std::string> params,
                           std::vector<std::unique_ptr<ast>> b)
    : name(n), parameters(std::move(params)), body(std::move(b)) {}
std::string FunctionNode::repr() { return "FunctionNode(" + name + ")"; }

PrintNode::PrintNode(std::unique_ptr<ast> e) : expr(std::move(e)) {}
std::string PrintNode::repr() { return "PrintNode("+ expr->repr() + ")";
}
