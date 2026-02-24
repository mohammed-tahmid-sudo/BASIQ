#include <ast.h>
#include <cctype>
#include <colors.h>
#include <cstdio>
#include <lexer.h>
#include <string>
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

std::vector<Token> Lexer::lexer() {
  std::vector<Token> out;

  while (true) {
    skipWhiwSpace();
    char c = Peek();
    if (c == 0)
      break;

    // Comments
    if (c == '/' && PeekNext() == '/') {
      // line comment
      Consume(); // '/'
      Consume(); // '/'
      while (Peek() != '\n' && Peek() != 0)
        Consume();
      continue;
    }
    if (c == '/' && PeekNext() == '*') {
      // block comment
      Consume(); // '/'
      Consume(); // '*'
      while (!(Peek() == '*' && PeekNext() == '/') && Peek() != 0) {
        Consume();
      }
      if (Peek() == '*') {
        Consume(); /* '*' */
        if (Peek() == '/')
          Consume();
      }
      continue;
    }

    // Headers: @version, @author, @import, @syscall
    if (c == '@') {
      Consume(); // consume '@'
      // read directive name
      std::string word;
      while (std::isalpha(Peek()))
        word.push_back(Peek()), Consume();
      std::string lw = toLower(word);
      Token t;
      if (lw == "version") {
        t.type = VERSION;
        t.value = word;
      } else if (lw == "author") {
        t.type = AUTHOR;
        t.value = word;
      } else if (lw == "import") {
        t.type = IMPORT;
        t.value = word;
      } else if (lw == "syscall") {
        t.type = SYSCALL;
        t.value = word;
      } else {
        // unknown header; treat as identifier-like
        t.type = IDENTIFIER;
        t.value = word;
      }
      out.push_back(t);
      continue;
    }

    // // String literal
    if (c == '"') {
      Consume(); // opening "
      std::string val;
      while (Peek() != '"' && Peek() != 0) {
        if (Peek() == '\\') {
          Consume();
          char esc = Peek();
          if (esc == 'n') {
            val.push_back('\n');
          } else if (esc == 't') {
            val.push_back('\t');
          } else if (esc == '"') {
            val.push_back('"');
          } else if (esc == '\\') {
            val.push_back('\\');
          } else
            val.push_back(esc);
          Consume();
        } else {
          val.push_back(Peek());
          Consume();
        }
      }
      if (Peek() == '"')
        Consume(); // closing "
      out.push_back({STRING_LITERAL, val});
      continue;
    }
    // Char literal

    // if (c == '\'') {
    //   Consume(); // opening '

    //   if (Peek() == '\'' || Peek() == 0) {
    //     // empty char literal or EOF → error
    //     out.push_back({CHAR_LITERAL, ""});
    //     Consume(); // avoid infinite loop if second '
    //     continue;
    //   }

    //   char value;

    //   if (Peek() == '\\') {
    //     Consume(); // '\'
    //     char esc = Peek();

    //     switch (esc) {
    //     case 'n':
    //       value = '\n';
    //       break;
    //     case 't':
    //       value = '\t';
    //       break;
    //     case '0':
    //       value = '\0';
    //       break;
    //     case '\'':
    //       value = '\'';
    //       break;
    //     case '\\':
    //       value = '\\';
    //       break;
    //     default:
    //       value = esc;
    //       break;
    //     }

    //     Consume();
    //   } else {
    //     value = Peek();
    //     Consume();
    //   }

    //   if (Peek() != '\'') {
    //     // missing closing quote → error
    //     out.push_back({CHAR_LITERAL, ""});
    //     continue;
    //   }

    //   Consume(); // closing '

    //   std::string s(1, value);
    //   out.push_back({CHAR_LITERAL, s});
    //   continue;
    // }
    // Numbers (int or float)
    if (std::isdigit(c)) {
      std::string num;
      while (std::isdigit(Peek())) {
        num.push_back(Peek());
        Consume();
      }
      if (Peek() == '.' && std::isdigit(PeekNext())) {
        num.push_back(Peek()); // '.'
        Consume();
        while (std::isdigit(Peek())) {
          num.push_back(Peek());
          Consume();
        }
        out.push_back({FLOAT_LITERAL, num});
      } else {
        out.push_back({INT_LITERAL, num});
      }
      continue;
    }

    // Identifiers / keywords / booleans / types
    if (std::isalpha(c) || c == '_') {
      std::string id;
      while (std::isalnum(Peek()) || Peek() == '_') {
        id.push_back(Peek());
        Consume();
      }

      std::string lower = toLower(id);

      // boolean literals (case-insensitive)
      if (lower == "true" || lower == "false") {
        out.push_back({BOOLEAN_LITERAL, id});
        continue;
      }

      // keywords (lowercase)
      if (lower == "let") {
        out.push_back({LET, id});
        continue;
      }
      if (lower == "func") {
        out.push_back({FUNC, id});
        continue;
      }
      if (lower == "return") {
        out.push_back({RETURN, id});
        continue;
      }
      if (lower == "if") {
        out.push_back({IF, id});
        continue;
      }
      if (lower == "else") {
        out.push_back({ELSE, id});
        continue;
      }
      if (lower == "for") {
        out.push_back({FOR, id});
        continue;
      }
      if (lower == "in") {
        out.push_back({IN, id});
        continue;
      }
      if (lower == "while") {
        out.push_back({WHILE, id});
        continue;
      }
      if (lower == "class") {
        out.push_back({CLASS, id});
        continue;
      }

      // Types (case-sensitive as per grammar: Integer, Float, Boolean, String)
      if (id == "Integer" || id == "Float" || id == "Boolean" ||
          id == "String" || id == "Void" || id == "Char") {
        out.push_back({TYPES, id});
        continue;
      }

      // default: identifier
      out.push_back({IDENTIFIER, id});
      continue;
    }

    // Two-char tokens
    if (c == '=' && PeekNext() == '=') {
      Consume();
      Consume();
      out.push_back({EQEQ, "=="});
      continue;
    }
    if (c == '!' && PeekNext() == '=') {
      Consume();
      Consume();
      out.push_back({NOTEQ, "!="});
      continue;
    }
    if (c == '<' && PeekNext() == '=') {
      Consume();
      Consume();
      out.push_back({LTE, "<="});
      continue;
    }
    if (c == '>' && PeekNext() == '=') {
      Consume();
      Consume();
      out.push_back({GTE, ">="});
      continue;
    }
    if (c == '&' && PeekNext() == '&') {
      Consume();
      Consume();
      out.push_back({AND, "&&"});
      continue;
    }
    if (c == '|' && PeekNext() == '|') {
      Consume();
      Consume();
      out.push_back({OR, "||"});
      continue;
    }
    if (c == '-' && PeekNext() == '>') {
      Consume();
      Consume();
      out.push_back({DASHGREATER, "->"});
      continue;
    }
    if (c == '.' && PeekNext() == '.') {
      Consume();
      Consume();
      out.push_back({RANGE, ".."});
      continue;
    }

    // Single-char operators / punctuation
    switch (c) {
    case '+':
      Consume();
      out.push_back({PLUS, "+"});
      break;
    case '-':
      Consume();
      out.push_back({MINUS, "-"});
      break;
    case '*':
      Consume();
      out.push_back({STAR, "*"});
      break;
    case '/':
      Consume();
      out.push_back({SLASH, "/"});
      break;
    case '=':
      Consume();
      out.push_back({EQ, "="});
      break;
    case '<':
      Consume();
      out.push_back({LT, "<"});
      break;
    case '>':
      Consume();
      out.push_back({GT, ">"});
      break;
    case '(':
      Consume();
      out.push_back({LPAREN, "("});
      break;
    case ')':
      Consume();
      out.push_back({RPAREN, ")"});
      break;
    case '{':
      Consume();
      out.push_back({LBRACE, "{"});
      break;
    case '}':
      Consume();
      out.push_back({RBRACE, "}"});
      break;
    case '[':
      Consume();
      out.push_back({LBRACKET, "["});
      break;
    case ']':
      Consume();
      out.push_back({RBRACKET, "]"});
      break;
    case ':':
      Consume();
      out.push_back({COLON, ":"});
      break;
    case ',':
      Consume();
      out.push_back({COMMA, ","});
      break;
    case ';':
      Consume();
      out.push_back({SEMICOLON, ";"});
      break;
    default:
      // Unknown/unsupported char: consume it to avoid infinite loop, but
      // produce IDENTIFIER with single char
      {
        std::string s;
        s.push_back(c);
        Consume();
        out.push_back({IDENTIFIER, s});
      }
      break;
    } // switch
  } // while

  // EOF token
  out.push_back({EOF_TOKEN, ""});
  return out;
}

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
  case BOOLEAN_LITERAL:
    return "BOOLEAN_LITERAL";
  case IDENTIFIER:
    return "IDENTIFIER";
  case INT_LITERAL:
    return "INT_LITERAL";
  case FLOAT_LITERAL:
    return "FLOAT_LITERAL";
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
  case DASHGREATER:
    return "DASHGREATER";
  case RANGE:
    return "RANGE";
  case EOF_TOKEN:
    return "EOF";
  case TYPES:
    return "Types";
  case SEMICOLON:
    return "SEMICOLON";
  case LBRACKET:
    return "LBRACKET";
  case RBRACKET:
    return "RBRACKET";
  case STRING_LITERAL:
    return "STRING_LITERAL";
  default:
    return "UNKNOWN";
  }
}

// int main() {
//   std::string src = R"(
//   @version "1.0";
//   @author "Tahmid";

//   let x: Integer = 10;
//   let y: Float = 3.14;

//   let y: Integer[2] = [21, 12];
//   ley something: Char{32} = {'a', 'b', 'c', 'd'. 'e' , '\0'};

//   func add(a: Integer, b: Integer) -> void {
// 	return a + b;
//   }

//   if x >= 5 {
// 	2 + 1;
//   } else {
// 	2 + 1;
//   }

//   for i in 0..10 {
// 	2 + 1;
//   }

//   )";

//   // std::string src = R"(
//   // let x:Integer;
//   // )";
//   Lexer lexer(src);
//   auto program = lexer.lexer();

//   int stmtNo = 0;
//   for (const auto &stmt : program) {
//     std::cout << "  " << std::setw(12) << tokenName(stmt.type) << " : '"
//               << stmt.value << "'\n";
//   }
// }
