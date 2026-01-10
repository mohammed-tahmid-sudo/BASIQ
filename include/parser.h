// parser.h
#pragma once
#include <algorithm>
#include <ast.h>
#include <memory>
#include <string>
#include <vector>

class Parser {
  int x = 0;
  int y = 0;

public:
  std::vector<std::vector<std::vector<std::string>>> input;

  Parser(decltype(input) in) : input(in) {}

  std::string Peek(int i);
  void consume();
  void expect(const std::string &t);

  std::vector<std::unique_ptr<ast>> Parse();

  std::unique_ptr<ast>
  parseStmt(std::vector<std::vector<std::vector<std::string>>> input);

  std::unique_ptr<ast>
  parseExpr(std::vector<std::vector<std::vector<std::string>>> input);

};
