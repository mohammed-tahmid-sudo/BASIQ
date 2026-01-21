// lexer.h
#pragma once

#include <string>
#include <vector>

enum class TokenType {
  // Keywords
  VAR,
  AS,
  IF,
  THEN,
  ELIF,
  ELSE,
  END,
  WHILE,
  DO,
  RETURN,
  PRINT,
  TRUE_KW,
  FALSE_KW,
  INTEGER_TYPE,
  FLOAT_TYPE,
  BOOLEAN_TYPE,
  CHAR_TYPE,
  STRING_TYPE,

  // Operators
  PLUS,
  MINUS,
  STAR,
  SLASH,
  EQUAL,

  // Delimiters
  LPAREN,
  RPAREN,
  COMMA,
  SEMICOLON,

  // Literals
  INTEGER_LITERAL,
  FLOAT_LITERAL,
  CHAR_LITERAL,
  STRING_LITERAL,
  BOOLEAN_LITERAL,

  // Identifier
  IDENTIFIER,

  // End of file/input
  EOF_TOKEN
};

struct Token {
  TokenType Type;
  std::string Value;
  Token() = default;

  Token(TokenType t, const std::string &v) : Type(t), Value(v) {}
};

std::string tokenTypeToString(TokenType type);

class Lexer {
public:
  std::vector<std::vector<Token>>
  lexerSplitStatements(const std::string &input);

private:
  size_t i = 0;

  char peek(const std::string &input);
  char get(const std::string &input);
  void skipWhitespace(const std::string &input);

  bool isKeyword(const std::string &s, TokenType &type);
  Token nextToken(const std::string &input);
};
