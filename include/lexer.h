#pragma  once 
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
  // Types
  INTEGER,
  FLOAT,
  BOOLEAN,
  STRING,
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

struct Token {
  TokenType type;
  std::string value;
};

class Lexer {
  std::vector<std::string> input;
  size_t index = 0;

public:
  Lexer(std::vector<std::string> inp) : input(inp) {}
  std::string Peek() const;
  std::string PeekNext() const;
  void consume();

  std::vector<std::vector<Token>> lexer();
};
