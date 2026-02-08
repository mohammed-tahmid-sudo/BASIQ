#pragma once
#include "lexer.h"
#include <algorithm>
#include <ast.h>
#include <memory>
#include <vector>

class Parser {
  std::vector<std::vector<Token>> code;
  size_t x = 0;
  size_t y = 0;

public:
  Parser(const std::vector<std::vector<Token>> &tokens) : code(tokens) {}

  Token Peek() const;
  Token Consume();
  Token Expect(TokenType expected);

  std::unique_ptr<ast> ParseExpression();

  std::unique_ptr<VariableDeclareNode> ParserVariable();
  std::unique_ptr<ast> ParseStatements();

  std::vector<std::unique_ptr<ast>> Parse();
};
