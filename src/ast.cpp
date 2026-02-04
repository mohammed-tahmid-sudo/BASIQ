#include <ast.h>
#include <string>

std::string IntegerNode::repr() {
  return "InegerNode(" + std::to_string(val) + ")";
}

std::string FloatNode::repr() {
  return "FloatNode(" + std::to_string(val) + ")";
}

std::string BooleanNode::repr() {
  return "BooleanNode(" + std::to_string(val) + ")";
}

std::string StringNode::repr() { return "StringNode(" + val + ")"; }
