#include <ast.h>
#include <lexer.h>
#include <string>

std::string TypesToString(Types tp) {
  switch (tp) {
  case INTEGERTYPE:
    return "Integer";
  case FLOATTYEP:
    return "Float";
  case BOOLEANTYPE:
    return "Boolean";
  case STRINGTYPE:
    return "String";
  case IDENTIFIERTYPE:
    return "Identifier";
  default:
    return "Unknown";
  }
}

std::string binaryOpTokenTypeToString(BinaryOpTokentype op) {
  switch (op) {
  case PLUSOP:
    return "+";
  case MINUSOP:
    return "-";
  case MULOP:
    return "*";
  case DIVOP:
    return "/";
  case EQUALOP:
    return "==";
  case NOTEQUALOP:
    return "!=";
  case LESSTHANOP:
    return "<";
  case GREATERTHANOP:
    return ">";
  case LESSTHANEQUALOP:
    return "<=";
  case GREATERTHANEQUALOP:
    return ">=";
  case ANDOP:
    return "&&";
  case OROP:
    return "||";
  default:
    return "Unknown";
  }
}

std::string IntLiteralNode::repr() {
  return "IntLiteralNode(" + std::to_string(val) + ")";
}

std::string FloatLiteralNode::repr() {
  return "FloatLiteralNode(" + std::to_string(val) + ")";
}

std::string StringLiteralNode::repr() {
  return "StringLiteralNode(\"" + val + "\")";
}

std::string BooleanLiteralNode::repr() {
  return "BooleanLiteralNode(" + std::string(val ? "true" : "false") + ")";
}

std::string BinaryOperationNode::repr() {
  return "BinaryOperationNode(Op=" +
         std::string(binaryOpTokenTypeToString(op)) + ", Left=" + left->repr() +
         ", Right=" + right->repr() + ")";
}

std::string VariableDeclareNode::repr() {
  return "VariableDeclareNode(Name=" + name + ", Type=" + TypesToString(type) +
         ", Content=" + (Content ? Content->repr() : "null") + ")";
}

std::string AssignmentNode::repr() {
  return "AssignmentNode(Name=" + name + ", Type=" + TypesToString(type) +
         ", Content=" + (Content ? Content->repr() : "null") + ")";
}

std::string FunctionNode::repr() {
  return "FunctionNode(Name=" + name +
         ", Contents=" + (contents ? contents->repr() : "null") + ")";
}

std::string CompountNode::repr() {
  std::string reprStr = "CompountNode(Contents=[";
  for (size_t i = 0; i < contents.size(); ++i) {
    reprStr += contents[i] ? contents[i]->repr() : "null";
    if (i + 1 < contents.size())
      reprStr += ", ";
  }
  reprStr += "])";
  return reprStr;
}

std::string IfNode::repr() {
  return "IfNode(Args=" + (args ? args->repr() : "null") +
         ", Block=" + (block ? block->repr() : "null") +
         ", ElseBlock=" + (elseBlock ? elseBlock->repr() : "null") + ")";
}

std::string WhileNode::repr() {
  return "WhileNode(Args=" + (args ? args->repr() : "null") +
         ", Block=" + (block ? block->repr() : "null") + ")";
}
