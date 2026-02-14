#include "ast.h"
#include "lexer.h"
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <llvm-18/llvm/IR/InstrTypes.h>
#include <llvm-18/llvm/IR/Instruction.h>
#include <llvm-18/llvm/Support/Error.h>
#include <memory>
#include <parser.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

Token Parser::Peek() {
  if (x < input.size()) {
    return input[x];
  }
  return {TokenType::EOF_TOKEN, ""};
}

Token Parser::Consume() {
  if (x < input.size()) {
    return input[x++]; // return current, then advance
  }
  return {TokenType::EOF_TOKEN, ""};
}

Token Parser::Expect(TokenType tk) {
  if (Peek().type == tk) {
    // std::cout << "COMMING HERE " << tokenName(tk) << std::endl;
    return Consume();
  }
  throw std::runtime_error("EXPECTED" + std::string(tokenName(tk)) + ", GOT " +
                           tokenName(Peek().type));
}

std::unique_ptr<ast> Parser::ParseFactor() {
  if (Peek().type == TokenType::INT_LITERAL) {
    int val = std::stoi(Peek().value);
    Consume();
    return std::make_unique<IntegerNode>(val);

  } else if (Peek().type == TokenType::FLOAT_LITERAL) {
    float val = std::stof(Peek().value);
    Consume();
    return std::make_unique<FloatNode>(val);

  } else if (Peek().type == TokenType::BOOLEAN_LITERAL) {

    if (Peek().value == "true") {
      Consume();
      return std::make_unique<BooleanNode>(true);
    }

    Consume();
    return std::make_unique<BooleanNode>(false);

  } else if (Peek().type == TokenType::STRING_LITERAL) {
    std::string val = Peek().value;
    Consume();
    return std::make_unique<StringNode>(val);

  } else if (Peek().type == TokenType::LPAREN) {
    Expect(TokenType::LPAREN);
    auto val = ParseExpression();
    if (!val) {
      throw std::runtime_error("VAL IS A NULLPTR");
    }
    Expect(TokenType::RPAREN);
    return val;
  }

  else {
    return nullptr;
  }
}

std::unique_ptr<ast> Parser::ParseTerm() {
  std::unique_ptr<ast> left = ParseFactor();
  while (Peek().type == TokenType::STAR || Peek().type == TokenType::SLASH) {
    TokenType type = Peek().type;
    Consume();

    std::unique_ptr<ast> right = ParseFactor();

    if (!right)
      throw std::runtime_error("EXPECTED A NUMBER AFTER * OR /");

    left = std::make_unique<BinaryOperationNode>(type, std::move(left),
                                                 std::move(right));
  }

  return left;
}

std::unique_ptr<ast> Parser::ParseExpression() {

  std::unique_ptr<ast> left = ParseTerm();
  while (Peek().type == TokenType::PLUS || Peek().type == TokenType::MINUS) {
    TokenType type = Peek().type;
    Consume();

    std::unique_ptr<ast> right = ParseTerm();

    if (!right)
      throw std::runtime_error("EXPECTED A NUMBER AFTER + OR -");

    left = std::make_unique<BinaryOperationNode>(type, std::move(left),
                                                 std::move(right));
  }

  return left;
}

std::unique_ptr<ast> Parser::ParseStatement() { return ParseExpression(); }
std::vector<std::unique_ptr<ast>> Parser::Parse() {
  std::vector<std::unique_ptr<ast>> output;

  while (Peek().type != TokenType::EOF_TOKEN) {
    auto stmt = ParseStatement();
    output.push_back(std::move(stmt));

    if (Peek().type == TokenType::SEMICOLON) {
      Consume(); // move past the semicolon
    } else {
      throw std::runtime_error("Expected ';' after statement");
    }
  }
  return output;
}
int main() {
  // std::string src = R"(
  // @version "1.0";
  // @author "Tahmid";

  // let x: Integer = 10;
  // let y: Float = 3.14;

  // func add(a: Integer, b: Integer) -> void {
  // return a + b;
  // }

  // if x >= 5 {
  // 2 + 1;
  // } else {
  // 2 + 1;
  // }

  // for i in 0..10 {
  // 2 + 1;
  // }

  // )";

  std::string src = R"(
  1 * 1 + (1 + 1); 
  1 * 1 + (1 + 1); 
  1 * 1 + (1 + 1); 
  1 * 1 + (1 + 1); 
  )";
  Lexer lexer(src);
  auto program = lexer.lexer();

  int stmtNo = 0;
  for (const auto &stmt : program) {
    std::cout << "  " << std::setw(12) << tokenName(stmt.type) << " : '"
              << stmt.value << "'\n";
  }

  std::cout << "---------------------------------------------------------------"
               "---------------------------------------------------------------"
               "---------------------------------------------------------------"
               "-----------------------"
            << std::endl;
  Parser parser(program, "MYMODULE");
  auto val = parser.Parse();
  for (auto &v : val) {
    std::cout << v->repr() << std::endl;
  }
}
