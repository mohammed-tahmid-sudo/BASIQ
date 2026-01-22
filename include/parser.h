// parser.h
#pragma once
#include "lexer.h"
#include <ast.h>
#include <functional>
#include <memory>
#include <string>
#include <vector>

class Parser {
  std::vector<std::vector<Token>> input;
  size_t i = 0;

  size_t x = 0;
  size_t y = 0;

  std::unique_ptr<ast>
  parseFuncExprFactor(std::function<std::unique_ptr<ast>()> func,
                      std::array<TokenType, 2> TokenType);

public:
  Parser(std::vector<std::vector<Token>> inp) : input(inp){};

  std::string Peek(bool i = false);
  void Consume();
  bool Expect(TokenType tokentype);

  std::unique_ptr<ast> ParseTerm();
  std::unique_ptr<ast> ParseFactor();
  std::unique_ptr<ast> ParseBinOP();
  std::unique_ptr<ast> ParseComparison();

  std::unique_ptr<ast> ParseExpressions();

  std::unique_ptr<IfNode> ParseIfElseStatement();
  std::unique_ptr<WhileNode> ParseWhileStatement();
  std::unique_ptr<VariableDeclareNode> ParseVariableStatement();
  std::unique_ptr<ast> ParseStatements();

  std::vector<std::unique_ptr<ast>> Parse();
};
