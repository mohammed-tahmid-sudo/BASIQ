// parser.h
#pragma once
#include "lexer.h"
#include <ast.h>
#include <memory>
#include <string>
#include <vector>

class Parser {
  std::vector<std::vector<Token>> input;
  size_t i = 0;

  size_t x = 0;
  size_t y = 0;

public:
  Parser(std::vector<std::vector<Token>> inp) : input(inp){};

  std::string Peek(bool i = false);
  void Consume();
  bool Expect(TokenType tokentype);
  std::unique_ptr<ast> ParseStatements();
  std::unique_ptr<ast> ParseExpressions();
  std::vector<std::unique_ptr<ast>> Parse();
};
