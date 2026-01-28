
#include <lexer.h>
#include <string>

std::string Lexer::Peek() const {
  return index < input.size() ? std::string(1, input[index]) : std::string();
}

std::string Lexer::PeekNext() const {
  return (index + 1) < input.size() ? std::string(1, input[index + 1])
                                    : std::string();
}

void Lexer::consume() {}
