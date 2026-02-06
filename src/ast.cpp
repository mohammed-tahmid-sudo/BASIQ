#include <ast.h>
#include <lexer.h>
#include <llvm-18/llvm/IR/Function.h>
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

std::string VariableDeclareNode::repr() {
  return "VariableDeclareNode(name= " + name + " ,value=" + val->repr() +
         ", Type=";
}

std::string AssignmentNode::repr() {
  return "AssignmentNode(name=" + name + ", NewValue=" + val->repr() + ")";
}

std::string CompoundNode::repr() {
  std::string output;
  output = +"[";
  for (auto &block : blocks) {
    std::string val = block->repr();
    output += ", " + val;
  }
  output = +"]";
  return output;
}

std::string FunctionNode::repr() {
  return "FunctionNode(Name=" + name + ", Value=" + content->repr() +
         "ReturnType=";
}
std::string VariableReferenceNode::repr() {
  return "VariableReferenceNode(" + Name + ")";
}

std::string WhileNode::repr() {
  return "WhileNode(Condition=" + condition->repr() + ", Body=" + body->repr() +
         ")";
}
