#include "ast.h"
#include <memory>
#include <utility>
#include <vector>

// -------- Expression / Value --------

NumberNode::NumberNode(int n) : number(n) {}
std::string NumberNode::repr() {
  return "NumberNode(" + std::to_string(number) + ")";
}

VariableNode::VariableNode(const std::string &n) : name(n) {}
std::string VariableNode::repr() { return "VariableNode(" + name + ")"; }

// Correct constructor
VariableDeclareNode::VariableDeclareNode(
    std::string n, std::string tp, std::unique_ptr<ast> cntnt)
    : name(n), type(std::move(tp)), contents(std::move(cntnt)) {}

std::string VariableDeclareNode::repr() {
  return "VariableDeclareNode(" + name + ", type=" + type + ")";
}

AssignmentNode::AssignmentNode(std::unique_ptr<VariableNode> n,
                               std::unique_ptr<ast> v,
                               const std::string &t)
    : name(std::move(n)), value(std::move(v)), type(t) {}
std::string AssignmentNode::repr() {
  std::string s = "AssignmentNode(" + name->repr() + ", value=[";
  for (auto &v : value)
    s += v->repr() + ",";
  s += "], type=" + type + ")";
  return s;
}

BinaryOperationNode::BinaryOperationNode(std::unique_ptr<ast> l,
                                         std::unique_ptr<ast> r, const char o)
    : left(std::move(l)), right(std::move(r)), op(o) {}
std::string BinaryOperationNode::repr() {
  return "BinaryOperationNode(op=" + std::string(1, op) +
         ", left=" + left->repr() + ", right=" + right->repr() + ")";
}

IdentifierNode::IdentifierNode(const std::string &n) : id(n) {}
std::string IdentifierNode::repr() { return "IdentifierNode(" + id + ")"; }

StringNode::StringNode(const std::string &v) : value(v) {}
std::string StringNode::repr() { return "StringNode(" + value + ")"; }

// -------- Control Flow --------

ReturnNode::ReturnNode(std::unique_ptr<ast> e) : expr(std::move(e)) {}
std::string ReturnNode::repr() {
  return "ReturnNode(" + (expr ? expr->repr() : "null") + ")";
}

std::string BreakNode::repr() { return "BreakNode"; }
std::string ContinueNode::repr() { return "ContinueNode"; }

ComparisonNode::ComparisonNode(std::vector<std::unique_ptr<ast>> l,
                               std::vector<std::unique_ptr<ast>> r,
                               const std::string &c)
    : left(std::move(l)), right(std::move(r)), comp(c) {}
std::string ComparisonNode::repr() {
  return "ComparisonNode(comp=" + comp + ")";
}

IfNode::IfNode(std::vector<std::unique_ptr<ast>> cond,
               std::vector<std::unique_ptr<ast>> ifBody,
               std::vector<std::unique_ptr<ast>> elseB)
    : condition(std::move(cond)), body(std::move(ifBody)),
      elseBody(std::move(elseB)) {}

std::string IfNode::repr() {
  std::string condStr, ifStr, elseStr;

  for (auto &c : condition)
    condStr += c->repr() + ", ";
  for (auto &b : body)
    ifStr += b->repr() + ", ";
  for (auto &e : elseBody)
    elseStr += e->repr() + ", ";

  // Remove trailing comma and space
  if (!condStr.empty())
    condStr.pop_back(), condStr.pop_back();
  if (!ifStr.empty())
    ifStr.pop_back(), ifStr.pop_back();
  if (!elseStr.empty())
    elseStr.pop_back(), elseStr.pop_back();

  return "IfNode(condition=[" + condStr + "], ifBody=[" + ifStr +
         "], elseBody=[" + elseStr + "])";
}

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

PrintNode::PrintNode(std::unique_ptr<ast> arg) : args(std::move(arg)) {}
std::string PrintNode::repr() { return "PrintNode(" + args->repr() + ")"; }
