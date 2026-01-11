// parser.h
#pragma once
#include <ast.h>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

class Parser {

public:
  size_t x = 0;
  size_t y = 0;

  std::vector<std::vector<std::vector<std::string>>> input;

  std::vector<std::unique_ptr<ast>>
  Parse(std::vector<std::vector<std::vector<std::string>>> input,
        bool parse_stmt = true);
};
