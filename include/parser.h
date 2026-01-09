#pragma once

#include <ast.h>
#include <string>
#include <vector>

class Parser {
  int x = 0;
  int y = 0;

public:
  std::vector<std::vector<std::vector<std::string>>> input;

  Parser(std::vector<std::vector<std::vector<std::string>>> inp) : input(inp){};

  std::string Peek(int i);

  void consume();

  std::vector<std::unique_ptr<ast>> Parse();
};
