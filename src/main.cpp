// src/main.cpp
// #include <lexer.h>

#include "lexer.cpp"
#include <ast.h>
#include <colors.h>
#include <iostream>
#include <ostream>
#include <parser.h>

int main() {
  std::cout << Colors::BOLD << Colors::GREEN << "LEXED VALUE" << Colors::RESET
            << "\n";

  Lexer lex;
  // auto tokens = lex.lexer("VAR x = 5 AS INTEGER; IF x > 3 THEN RETURN x;
  // END;");

  std::vector<std::vector<std::vector<std::string>>> fullcode;

  fullcode.push_back(lex.lexer("VAR a = 5 AS INTEGER"));
  fullcode.push_back(lex.lexer("VAR b = 5 AS INTEGER"));
  fullcode.push_back(lex.lexer("WHILE a + b DO "));
  fullcode.push_back(lex.lexer("a - b END"));

  for (auto &toks : fullcode) {
    for (auto &tok : toks) {
      std::cout << tok[0] << " : " << tok[1] << std::endl;
    }
  }

  std::cout << Colors::BOLD << Colors::GREEN << "PARSED VALUE" << Colors::RESET
            << "\n";

  Parser parse;

  std::vector<std::unique_ptr<ast>> parsed_output = parse.Parse(fullcode);
  for (auto &p : parsed_output) {
    std::cout << p->repr() << "\n";
  }

  std::cout << "\n";
}
