#pragma once

#include <ast.h>
#include <vector>

class Parser {
public:
  std::vector<std::unique_ptr<ast>>
  Parse(std::vector<std::vector<std::vector<std::string>>> input);
};
