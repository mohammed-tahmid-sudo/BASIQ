#include "lexer.h"
#include <ast.h>
#include <colors.h>
#include <functional>
#include <iostream>
#include <llvm-18/llvm/ADT/STLExtras.h>
#include <llvm-18/llvm/Support/AtomicOrdering.h>
#include <memory>
#include <ostream>
#include <parser.h>
#include <string>
#include <vector>

std::string Parser::Peek(bool value) {
  if (x >= input.size() || y >= input[x].size())
    return "";

  if (!value) {
    return tokenTypeToString(input[x][y].Type);
  } else {
    return input[x][y].Value;
  }
}

void Parser::Consume() {
  if (x >= input.size())
    return;
  y++;
  if (y >= input[x].size()) {
    y = 0;
    x++;
  }
}

bool Parser::Expect(TokenType tok) {
  if (Peek() != tokenTypeToString(tok)) {
    std::cerr << "Expected " << tokenTypeToString(tok) << ", got " << Peek(true)
              << "\n";
    return false;
  }
  Consume();
  return true;
}

std::unique_ptr<ast>
Parser::parseFuncExprFactor(std::function<std::unique_ptr<ast>()> func,
                            std::array<TokenType, 2> TokenType) {

  std::unique_ptr<ast> left = func();

  if (!left)
    return nullptr;

  while (Peek() == tokenTypeToString(TokenType[0]) ||
         Peek() == tokenTypeToString(TokenType[1])) {
    std::string t = Peek(true);
    Consume();
    std::unique_ptr<ast> right = func();
    if (!right)
      return nullptr;

    left = std::make_unique<BinaryOperationNode>(std::move(left),
                                                 std::move(right), t[0]);
  }

  return left;
}

std::unique_ptr<ast> Parser::ParseFactor() {

  if (Peek() == tokenTypeToString(TokenType::INTEGER_LITERAL) ||
      Peek() == tokenTypeToString(TokenType::FLOAT_LITERAL)) {

    std::string val = Peek(true);
    Consume();
    return std::make_unique<NumberNode>(std::stod(val));

  } else if (Peek() == tokenTypeToString(TokenType::LPAREN)) {

    Consume();
    auto node = ParseExpressions();
    Consume();
    return node;
  }

  return nullptr;
}

std::unique_ptr<ast> Parser::ParseTerm() {

  std::array<TokenType, 2> tokens = {TokenType::STAR, TokenType::SLASH};
  return parseFuncExprFactor([this]() { return ParseFactor(); }, tokens);
}
std::unique_ptr<ast> Parser::ParseExpressions() {

  std::array<TokenType, 2> tokens = {TokenType::PLUS, TokenType::MINUS};
  return parseFuncExprFactor([this]() { return ParseTerm(); }, tokens);
}

std::unique_ptr<IfNode> Parser::ParseIfElseStatement() {
  std::unique_ptr<IfNode> output = nullptr;
  Expect(TokenType::IF);

  Expect(TokenType::LPAREN);
  std::unique_ptr<ast> condition = ParseExpressions();
  Expect(TokenType::RPAREN);

  Expect(TokenType::THEN);
  std::vector<std::unique_ptr<ast>> args = Parser::Parse();
  Expect(TokenType::END);

  output = std::make_unique<IfNode>(std::move(condition), std::move(args));

  auto current = output.get();

  std::cout << Peek() << std::endl;

  while (Peek() == tokenTypeToString(TokenType::ELIF)) {
    Consume();

    auto elifCondition = ParseExpressions();
    auto elifBody = Parser::Parse();
    auto elifNode =
        std::make_unique<IfNode>(std::move(elifCondition), std::move(elifBody));

    current->elseBody.push_back(std::move(elifNode));
    current = static_cast<IfNode *>(current->elseBody.back().get());
  }

  if (Peek() == tokenTypeToString(TokenType::ELSE)) {
    Consume();
    auto elseBlock = Parser::Parse();
    current->elseBody = std::move(elseBlock);
  }

  return output;
}

std::unique_ptr<ast> Parser::ParseStatements() {
  std::unique_ptr<ast> output;

  if (Peek() == tokenTypeToString(TokenType::IF)) {
    output = ParseIfElseStatement();
  }

  return output;
}

std::vector<std::unique_ptr<ast>> Parser::Parse() {
  std::vector<std::unique_ptr<ast>> output;

  auto stmt = ParseStatements();
  if (stmt) {
    output.push_back(std::move(stmt));
  } else {
    auto expr = ParseExpressions();
    if (expr) {
      output.push_back(std::move(expr));
    }
  }

  return output;
}

int main() {
  Lexer lex;
  std::vector<std::vector<Token>> statements = lex.lexerSplitStatements(
      "IF (1) THEN 1  + 1 END ELIF (2) THEN 1 - 1 END ");

  for (size_t idx = 0; idx < statements.size(); ++idx) {
    for (const auto &tok : statements[idx]) {
      std::cout << "  " << tokenTypeToString(tok.Type) << " : " << tok.Value
                << "\n";
    }
    std::cout << Colors::BOLD << Colors::RED << "---------------------\n"
              << Colors::RESET;
  }
  std::cout << Colors::BOLD << Colors::GREEN << "---------------------"
            << "PARSER" << "---------------------\n"
            << Colors::RESET;
  Parser parse(statements);

  auto output = parse.Parse();

  for (auto &tok : output) {
    std::cout << tok->repr() << std::endl;
  }
}
