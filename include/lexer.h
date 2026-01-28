

#include <string>
#include <vector>

enum TkenType {
  INTEGER_VAL,
  FLOAT_VAL,
  STRING_VAL,
  TRUE,
  FAlSE,
  IDENTIFIER,
  STAR,
  SLASH,
  PLUS,
  MINUS,
  PRINT,
  LPAREN,
  RPAREN,
  RETURN,
  WHILE,
  DO,
  END,
  IF,
  THEN,
  ELSE,
  INTEGER,
  FLOAT,
  BOOLEAN,
  STRING,
  VAR, 
  AS, 
  

};

class Lexer {
  std::vector<std::string> input;
  size_t index = 0;

  std::string Peek();
  void consume();

public:
  Lexer(std::vector<std::string> inp) : input(inp) {}

  // std::vector<std::vector<Token>>
};
