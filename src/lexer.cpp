// lexer.cpp
#include <cctype>
#include <lexer.h>
#include <string>

std::string tokenTypeToString(TokenType type) {
  switch (type) {
  case TokenType::VAR:
    return "VAR";
  case TokenType::AS:
    return "AS";
  case TokenType::IF:
    return "IF";
  case TokenType::THEN:
    return "THEN";
  case TokenType::ELSE:
    return "ELSE";
  case TokenType::END_IF:
    return "END_IF";
  case TokenType::WHILE:
    return "WHILE";
  case TokenType::DO:
    return "DO";
  case TokenType::END_WHILE:
    return "END_WHILE";
  case TokenType::RETURN:
    return "RETURN";
  case TokenType::PRINT:
    return "PRINT";
  case TokenType::TRUE_KW:
    return "TRUE";
  case TokenType::FALSE_KW:
    return "FALSE";
  case TokenType::INTEGER_TYPE:
    return "INTEGER_TYPE";
  case TokenType::FLOAT_TYPE:
    return "FLOAT_TYPE";
  case TokenType::BOOLEAN_TYPE:
    return "BOOLEAN_TYPE";
  case TokenType::CHAR_TYPE:
    return "CHAR_TYPE";
  case TokenType::STRING_TYPE:
    return "STRING_TYPE";
  case TokenType::PLUS:
    return "PLUS";
  case TokenType::MINUS:
    return "MINUS";
  case TokenType::STAR:
    return "STAR";
  case TokenType::SLASH:
    return "SLASH";
  case TokenType::EQUAL:
    return "EQUAL";
  case TokenType::LPAREN:
    return "LPAREN";
  case TokenType::RPAREN:
    return "RPAREN";
  case TokenType::COMMA:
    return "COMMA";
  case TokenType::SEMICOLON:
    return "SEMICOLON";
  case TokenType::INTEGER_LITERAL:
    return "INTEGER_LITERAL";
  case TokenType::FLOAT_LITERAL:
    return "FLOAT_LITERAL";
  case TokenType::CHAR_LITERAL:
    return "CHAR_LITERAL";
  case TokenType::STRING_LITERAL:
    return "STRING_LITERAL";
  case TokenType::BOOLEAN_LITERAL:
    return "BOOLEAN_LITERAL";
  case TokenType::IDENTIFIER:
    return "IDENTIFIER";
  case TokenType::EOF_TOKEN:
    return "EOF";
  default:
    return "UNKNOWN";
  }
}

// helpers
char Lexer::peek(const std::string &input) {
  return i < input.size() ? input[i] : '\0';
}

char Lexer::get(const std::string &input) {
  return i < input.size() ? input[i++] : '\0';
}

void Lexer::skipWhitespace(const std::string &input) {
  while (std::isspace(peek(input)))
    i++;
}

bool Lexer::isKeyword(const std::string &s, TokenType &type) {
  if (s == "VAR")
    type = TokenType::VAR;
  else if (s == "AS")
    type = TokenType::AS;
  else if (s == "IF")
    type = TokenType::IF;
  else if (s == "THEN")
    type = TokenType::THEN;
  else if (s == "ELSE")
    type = TokenType::ELSE;
  else if (s == "WHILE")
    type = TokenType::WHILE;
  else if (s == "DO")
    type = TokenType::DO;
  else if (s == "RETURN")
    type = TokenType::RETURN;
  else if (s == "PRINT")
    type = TokenType::PRINT;
  else if (s == "TRUE")
    type = TokenType::BOOLEAN_LITERAL;
  else if (s == "FALSE")
    type = TokenType::BOOLEAN_LITERAL;
  else if (s == "INTEGER")
    type = TokenType::INTEGER_TYPE;
  else if (s == "FLOAT")
    type = TokenType::FLOAT_TYPE;
  else if (s == "BOOLEAN")
    type = TokenType::BOOLEAN_TYPE;
  else if (s == "CHAR")
    type = TokenType::CHAR_TYPE;
  else if (s == "STRING")
    type = TokenType::STRING_TYPE;
  else
    return false;
  return true;
}

Token Lexer::nextToken(const std::string &input) {
  skipWhitespace(input);
  char c = peek(input);

  if (c == '\0')
    return {TokenType::EOF_TOKEN, ""};

  // number
  if (std::isdigit(c)) {
    std::string num;
    bool isFloat = false;
    while (std::isdigit(peek(input)) || peek(input) == '.') {
      if (peek(input) == '.')
        isFloat = true;
      num += get(input);
    }
    return {isFloat ? TokenType::FLOAT_LITERAL : TokenType::INTEGER_LITERAL,
            num};
  }

  // identifier / keyword
  if (std::isalpha(c)) {
    std::string id;
    while (std::isalnum(peek(input)) || peek(input) == '_')
      id += get(input);

    TokenType type;
    if (isKeyword(id, type))
      return {type, id};
    return {TokenType::IDENTIFIER, id};
  }

  // string
  if (c == '"') {
    get(input);
    std::string s;
    while (peek(input) != '"' && peek(input) != '\0')
      s += get(input);
    get(input);
    return {TokenType::STRING_LITERAL, s};
  }

  // char
  if (c == '\'') {
    get(input);
    char ch = get(input);
    get(input);
    return {TokenType::CHAR_LITERAL, std::string(1, ch)};
  }

  // single-char tokens
  switch (c) {
  case '+':
    return {TokenType::PLUS, std::string(1, get(input))};
  case '-':
    return {TokenType::MINUS, std::string(1, get(input))};
  case '*':
    return {TokenType::STAR, std::string(1, get(input))};
  case '/':
    return {TokenType::SLASH, std::string(1, get(input))};
  case '=':
    return {TokenType::EQUAL, std::string(1, get(input))};
  case '(':
    return {TokenType::LPAREN, std::string(1, get(input))};
  case ')':
    return {TokenType::RPAREN, std::string(1, get(input))};
  case ',':
    return {TokenType::COMMA, std::string(1, get(input))};
  case ';':
    return {TokenType::SEMICOLON, std::string(1, get(input))};
  default:
    get(input); // skip unknown
    return nextToken(input);
  }
}

std::vector<std::vector<Token>>
Lexer::lexerSplitStatements(const std::string &input) {
  i = 0;

  std::vector<std::vector<Token>> statements;
  std::vector<Token> current;

  while (true) {
    Token tok = nextToken(input);
    if (tok.Type == TokenType::EOF_TOKEN)
      break;

    if (tok.Type == TokenType::SEMICOLON) {
      if (!current.empty()) {
        current.push_back(Token(TokenType::EOF_TOKEN, std::string("EOF")));
        statements.push_back(current);
      }
      current.clear();
    } else {
      current.push_back(tok);
    }
  }

  if (!current.empty())
    statements.push_back(current);

  return statements;
}
