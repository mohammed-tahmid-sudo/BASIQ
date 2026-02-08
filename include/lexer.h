#pragma once
#include <string>
#include <vector>

enum TokenType {
  // Headers
  VERSION,
  AUTHOR,
  IMPORT,
  SYSCALL,
  // Keywords
  LET,
  FUNC,
  RETURN,
  IF,
  ELSE,
  FOR,
  IN,
  WHILE,
  CLASS,
  PRINT,
  TRUE,
  FALSE,
  Types,
  INTEGER,
  FLOAT,
  BOOLEAN,
  STRING,
  VOID, 
  IDENTIFIER,

  // Literals
  INT_LITERAL,
  FLOAT_LITERAL,
  STRING_LITERAL,
  // Operators
  PLUS,
  MINUS,
  STAR,
  SLASH,
  EQ,
  EQEQ,
  NOTEQ,
  LT,
  GT,
  LTE,
  GTE,
  AND,
  OR,
  // Punctuation
  LPAREN,
  RPAREN,
  LBRACE,
  RBRACE,
  COLON,
  COMMA,
  DOT,
  RANGE,
  // End of file
  EOF_TOKEN
};
const char *tokenName(TokenType t);

struct Token {
  TokenType type;
  std::string value;
};

class Lexer {
  std::string input;
  size_t index = 0;

public:
  Lexer(std::string inp) : input(inp) {}
  char Peek() const;
  char PeekNext() const;
  void Consume();
  void skipWhiwSpace();
  static std::string toLower(const std::string &s) {
    std::string r;
    r.reserve(s.size());
    for (char c : s)
      r.push_back(std::tolower(static_cast<unsigned char>(c)));
    return r;
  }

  std::vector<std::vector<Token>> lexer();
};
