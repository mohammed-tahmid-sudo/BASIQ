#include <ast.h>
#include <lexer.h>
#include <llvm-18/llvm/IR/Function.h>
#include <sstream>
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
  return "VariableDeclareNode(name= " + name +
         " ,value=" + (val ? val->repr() : "null") + ", Type=" + Type.value +
         ")";
}

std::string AssignmentNode::repr() {
  return "AssignmentNode(name=" + name + ", NewValue=" + val->repr() + ")";
}

std::string CompoundNode::repr() {
  std::string output = "[";

  bool first = true;
  for (auto &block : blocks) {
    if (!first)
      output += ", ";
    output += block->repr();
    first = false;
  }

  output += "]";
  return output;
}

// std::string CompoundNode::repr() {
//   std::string output;
//   output = +"[";
//   for (auto &block : blocks) {
//     std::string val = block->repr();
//     output += ", " + val;
//   }
//   output = +"]";
//   return output;
// }

std::string FunctionNode::repr() {
  return "FunctionNode(Name=" + name + ", Value=[" + content->repr() +
         "], ReturnType=" + ReturnType.value;
}
std::string VariableReferenceNode::repr() {
  return "VariableReferenceNode(" + Name + ")";
}

std::string WhileNode::repr() {
  return "WhileNode(Condition=" + condition->repr() + ", Body=" + body->repr() +
         ")";
}

std::string IfNode::repr() {
  return "IfNode(Condition=" + condition->repr() +
         ", ThenBlock=" + thenBlock->repr() +
         ", EndBlock=" + elseBlock->repr() + ")";
}
std::string ReturnNode::repr() { return "ReturnNode(" + expr->repr() + ")"; }

std::string BinaryOperationNode::repr() {
  std::ostringstream oss;
  oss << "BinaryOperationNode(Op=" << tokenName(Type)
      << ", Left=" << Left->repr() << ", Right=" << Right->repr() << ")";
  return oss.str();
}

std::string BreakNode::repr() { return "BreakNode(NOARGS)"; }
std::string ContinueNode::repr() { return "ContinueNode(NOARGS)"; }

std::string CallNode::repr() { return "CallNode(Name=" + name + ", contents"; }
