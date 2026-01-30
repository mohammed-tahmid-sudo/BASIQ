#include <ast.h>
#include <cctype>
#include <lexer.h>
#include <memory>
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
    if (!cur.empty()) {
      output.push_back(cur);
      cur.clear();
    } else {
      // still push empty statement if you want; currently skip empties
    }
  };

  // keyword / type maps (lowercase -> TokenType)
  std::unordered_map<std::string, TokenType> kw{
      {"let", LET},
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
      // types (support both lowercase and capitalized from grammar)
      {"integer", INTEGER},
      {"float", FLOAT},
      {"boolean", BOOLEAN},
      {"string", STRING},
      // logical words
      {"and", AND},
      {"or", OR}};

  while (Peek() != 0) {
    skipWhiwSpace();
    char c = Peek();
    if (c == 0)
      break;

    // semicolon ends a statement (do not emit semicolon token)
    if (c == ';') {
      Consume();
      flush_statement();
      continue;
    }

    // header start '@'
    if (c == '@') {
      Consume(); // skip '@'
      // read identifier
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
        cur.push_back({IDENTIFIER, id}); // unknown header -> identifier
      continue;
    }

    // strings
    if (c == '"') {
      Consume(); // skip opening "
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
        Consume(); // skip closing "
      cur.push_back({STRING_LITERAL, val});
      continue;
    }

    // numbers (int or float)
    if (std::isdigit(static_cast<unsigned char>(c))) {
      std::string num;
      while (std::isdigit(static_cast<unsigned char>(Peek()))) {
        num.push_back(Peek());
        Consume();
      }
      // fractional part
      if (Peek() == '.' &&
          std::isdigit(static_cast<unsigned char>(PeekNext()))) {
        num.push_back(Peek());
        Consume(); // consume '.'
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

    // identifier or keyword (starts with letter)
    if (std::isalpha(static_cast<unsigned char>(c))) {
      std::string id;
      while (std::isalnum(static_cast<unsigned char>(Peek())) ||
             Peek() == '_') {
        id.push_back(Peek());
        Consume();
      }
      std::string idl = toLower(id);
      auto it = kw.find(idl);
      if (it != kw.end()) {
        cur.push_back({it->second, id});
      } else {
        cur.push_back({IDENTIFIER, id});
      }
      continue;
    }

    // two-char operators first
    char n = PeekNext();
    // '==' '!=' '<=' '>=' '..' '->' '&&' '||'
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
    } // grammar uses "->" for return type. reuse DOT token? user didn't include
      // ARROW; use DOT token with value "->"
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
      // unknown single char -> treat as identifier char fallback or skip
      {
        std::string s(1, c);
        cur.push_back({IDENTIFIER, s});
        Consume();
      }
      break;
    }
  } // while

  // push final statement if any tokens remain
  if (!cur.empty())
    output.push_back(cur);

  // Optionally append EOF token as its own statement
  std::vector<Token> eofVec;
  eofVec.push_back({EOF_TOKEN, ""});
  output.push_back(eofVec);

  return output;
}
#include <iomanip>
#include <iostream>

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
  default:
    return "UNKNOWN";
  }
}

// int main() {
//   std::string src = R"(
// @version "1.0";
// @author "Tahmid";

// let x: Integer = 10; const let y: Float = 3.14;

// func add(a: Integer, b: Integer) -> Integer {
//   return a + b;
// };

// if x >= 5 {
//   print ("ok");
// } else {
//   print ("no");
// };

// for i in 0..10 {
//   print (i);
// };

// )";

//   Lexer lexer(src);
//   auto program = lexer.lexer();

//   int stmtNo = 0;
//   for (const auto &stmt : program) {
//     std::cout << "Statement " << stmtNo++ << ":\n";
//     for (const auto &tok : stmt) {
//       std::cout << "  " << std::setw(12) << tokenName(tok.type) << " : '"
//                 << tok.value << "'\n";
//     }
//     std::cout << "\n";
//   }
// }

