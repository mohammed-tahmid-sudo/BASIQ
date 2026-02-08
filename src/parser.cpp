#include <ast.h>
#include <llvm-18/llvm/Support/CommandLine.h>
#include <memory>
#include <parser.h>
#include <vector>

Token Parser::Peek() const {

  if (x >= code.size()) {
    return Token{TokenType::EOF_TOKEN, ""}; // end of all code
  }
  if (y >= code[x].size()) {
    // end of this line, return EOF_TOKEN for this line
    return Token{TokenType::EOF_TOKEN, ""};
  }
  return code[x][y];
}

Token Parser::Consume() {
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

std::vector<std::unique_ptr<typename Tp>>
