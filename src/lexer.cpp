#include <ast.h>
#include <cctype>
#include <lexer.h>
#include <string>
#include <unordered_map>
#include <vector>

char Lexer::Peek() const { return index < input.size() ? input[index] : 0; }

char Lexer::PeekNext() const {
  return (index + 1) < input.size() ? input[index + 1] : 0;
}

void Lexer::Consume() {
  if (index < input.size())
    index++;
}
void Lexer::skipWhiwSpace() {
  while (std::isspace(Peek()))
    Consume();
}

std::vector<std::vector<Token>> Lexer::lexer() {
  std::vector<std::vector<Token>> output;
  std::vector<Token> cur;

  auto flush_statement = [&]() {
    // Always append EOF token at the end of statement, even if empty
    cur.push_back({EOF_TOKEN, ""});
    output.push_back(cur);
    cur.clear();
  };

  // keyword / type maps (lowercase -> TokenType)
  std::unordered_map<std::string, TokenType> kw{{"let", LET},
                                                {"func", FUNC},
                                                {"return", RETURN},
                                                {"if", IF},
                                                {"else", ELSE},
                                                {"for", FOR},
                                                {"in", IN},
                                                {"while", WHILE},
                                                {"class", CLASS},
                                                {"print", PRINT},
                                                {"true", TRUE},
                                                {"false", FALSE},
                                                // types
                                                {"integer", INTEGER},
                                                {"float", FLOAT},
                                                {"boolean", BOOLEAN},
                                                {"string", STRING},
                                                {"void", VOID},
                                                // logical words
                                                {"and", AND},
                                                {"or", OR}};

  while (Peek() != 0) {
    skipWhiwSpace();
    char c = Peek();
    if (c == 0)
      break;

    // semicolon ends a statement
    if (c == ';') {
      Consume();
      flush_statement();
      continue;
    }

    // header start '@'
    if (c == '@') {
      Consume(); // skip '@'
      std::string id;
      while (std::isalpha(static_cast<unsigned char>(Peek()))) {
        id.push_back(Peek());
        Consume();
      }
      std::string idl = toLower(id);
      if (idl == "version")
        cur.push_back({VERSION, id});
      else if (idl == "author")
        cur.push_back({AUTHOR, id});
      else if (idl == "import")
        cur.push_back({IMPORT, id});
      else if (idl == "syscall")
        cur.push_back({SYSCALL, id});
      else
        cur.push_back({IDENTIFIER, id});
      continue;
    }

    // strings
    if (c == '"') {
      Consume();
      std::string val;
      while (Peek() != 0 && Peek() != '"') {
        if (Peek() == '\\') {
          Consume();
          char esc = Peek();
          if (esc == 'n') {
            val.push_back('\n');
            Consume();
          } else if (esc == 't') {
            val.push_back('\t');
            Consume();
          } else if (esc == 'r') {
            val.push_back('\r');
            Consume();
          } else {
            if (esc != 0) {
              val.push_back(esc);
              Consume();
            }
          }
        } else {
          val.push_back(Peek());
          Consume();
        }
      }
      if (Peek() == '"')
        Consume();
      cur.push_back({STRING_LITERAL, val});
      continue;
    }

    // numbers
    if (std::isdigit(static_cast<unsigned char>(c))) {
      std::string num;
      while (std::isdigit(static_cast<unsigned char>(Peek()))) {
        num.push_back(Peek());
        Consume();
      }
      if (Peek() == '.' &&
          std::isdigit(static_cast<unsigned char>(PeekNext()))) {
        num.push_back(Peek());
        Consume();
        while (std::isdigit(static_cast<unsigned char>(Peek()))) {
          num.push_back(Peek());
          Consume();
        }
        cur.push_back({FLOAT_LITERAL, num});
      } else {
        cur.push_back({INT_LITERAL, num});
      }
      continue;
    }

    // identifiers / keywords
    if (std::isalpha(static_cast<unsigned char>(c))) {
      std::string id;
      while (std::isalnum(static_cast<unsigned char>(Peek())) ||
             Peek() == '_') {
        id.push_back(Peek());
        Consume();
      }
      std::string idl = toLower(id);
      auto it = kw.find(idl);
      if (it != kw.end())
        cur.push_back({it->second, id});
      else
        cur.push_back({IDENTIFIER, id});
      continue;
    }

    // two-char operators
    char n = PeekNext();
    if (c == '=' && n == '=') {
      Consume();
      Consume();
      cur.push_back({EQEQ, "=="});
      continue;
    }
    if (c == '!' && n == '=') {
      Consume();
      Consume();
      cur.push_back({NOTEQ, "!="});
      continue;
    }
    if (c == '<' && n == '=') {
      Consume();
      Consume();
      cur.push_back({LTE, "<="});
      continue;
    }
    if (c == '>' && n == '=') {
      Consume();
      Consume();
      cur.push_back({GTE, ">="});
      continue;
    }
    if (c == '.' && n == '.') {
      Consume();
      Consume();
      cur.push_back({RANGE, ".."});
      continue;
    }
    if (c == '-' && n == '>') {
      Consume();
      Consume();
      cur.push_back({DOT, "->"});
      continue;
    }
    if (c == '&' && n == '&') {
      Consume();
      Consume();
      cur.push_back({AND, "&&"});
      continue;
    }
    if (c == '|' && n == '|') {
      Consume();
      Consume();
      cur.push_back({OR, "||"});
      continue;
    }

    // single-char tokens
    switch (c) {
    case '+':
      cur.push_back({PLUS, "+"});
      Consume();
      break;
    case '-':
      cur.push_back({MINUS, "-"});
      Consume();
      break;
    case '*':
      cur.push_back({STAR, "*"});
      Consume();
      break;
    case '/':
      cur.push_back({SLASH, "/"});
      Consume();
      break;
    case '=':
      cur.push_back({EQ, "="});
      Consume();
      break;
    case '<':
      cur.push_back({LT, "<"});
      Consume();
      break;
    case '>':
      cur.push_back({GT, ">"});
      Consume();
      break;
    case '(':
      cur.push_back({LPAREN, "("});
      Consume();
      break;
    case ')':
      cur.push_back({RPAREN, ")"});
      Consume();
      break;
    case '{':
      cur.push_back({LBRACE, "{"});
      Consume();
      break;
    case '}':
      cur.push_back({RBRACE, "}"});
      Consume();
      break;
    case ':':
      cur.push_back({COLON, ":"});
      Consume();
      break;
    case ',':
      cur.push_back({COMMA, ","});
      Consume();
      break;
    case '.':
      cur.push_back({DOT, "."});
      Consume();
      break;
    default:
      cur.push_back({IDENTIFIER, std::string(1, c)});
      Consume();
      break;
    }
  } // while

  // flush last statement if any tokens remain
  if (!cur.empty())
    flush_statement();

  return output;
}

// assume lexer code is included above

const char *tokenName(TokenType t) {
  switch (t) {
  case VERSION:
    return "VERSION";
  case AUTHOR:
    return "AUTHOR";
  case IMPORT:
    return "IMPORT";
  case SYSCALL:
    return "SYSCALL";
  case LET:
    return "LET";
  case FUNC:
    return "FUNC";
  case RETURN:
    return "RETURN";
  case IF:
    return "IF";
  case ELSE:
    return "ELSE";
  case FOR:
    return "FOR";
  case IN:
    return "IN";
  case WHILE:
    return "WHILE";
  case CLASS:
    return "CLASS";
  case PRINT:
    return "PRINT";
  case TRUE:
    return "TRUE";
  case FALSE:
    return "FALSE";
  case INTEGER:
    return "INTEGER";
  case FLOAT:
    return "FLOAT";
  case BOOLEAN:
    return "BOOLEAN";
  case STRING:
    return "STRING";
  case IDENTIFIER:
    return "IDENTIFIER";
  case INT_LITERAL:
    return "INT_LITERAL";
  case FLOAT_LITERAL:
    return "FLOAT_LITERAL";
  case STRING_LITERAL:
    return "STRING_LITERAL";
  case PLUS:
    return "PLUS";
  case MINUS:
    return "MINUS";
  case STAR:
    return "STAR";
  case SLASH:
    return "SLASH";
  case EQ:
    return "EQ";
  case EQEQ:
    return "EQEQ";
  case NOTEQ:
    return "NOTEQ";
  case LT:
    return "LT";
  case GT:
    return "GT";
  case LTE:
    return "LTE";
  case GTE:
    return "GTE";
  case AND:
    return "AND";
  case OR:
    return "OR";
  case LPAREN:
    return "LPAREN";
  case RPAREN:
    return "RPAREN";
  case LBRACE:
    return "LBRACE";
  case RBRACE:
    return "RBRACE";
  case COLON:
    return "COLON";
  case COMMA:
    return "COMMA";
  case DOT:
    return "DOT";
  case RANGE:
    return "RANGE";
  case EOF_TOKEN:
    return "EOF";
  case VOID:
    return "Void";
  default:
    return "UNKNOWN";
  }
}
