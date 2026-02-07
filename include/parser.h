#pragma once
#include "lexer.h"
#include <vector>

class Parser {
  std::vector<std::vector<Token>> code;
  size_t x = 0;
  size_t y = 0;

public:
  Parser(const std::vector<std::vector<Token>> &tokens) : code(tokens) {}

  // Peek at the current token without advancing
  Token Peek() const {
    if (x >= code.size()) {
      return Token{TokenType::EOF_TOKEN, ""}; // end of all code
    }
    if (y >= code[x].size()) {
      // end of this line, return EOF_TOKEN for this line
      return Token{TokenType::EOF_TOKEN, ""};
    }
    return code[x][y];
  }

  // Consume the current token and advance
  Token Consume() {
    if (x >= code.size()) {
      return Token{TokenType::EOF_TOKEN, ""};
    }

    Token current = code[x][y];
    y++;

    // If we reach end of the current line, move to next line
    if (y >= code[x].size()) {
      x++;
      y = 0;
    }

    return current;
  }


};
