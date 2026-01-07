// src/main.cpp
#include <parser.h>
#include <ast.h>
#include <lexer.h>
#include <colors.h>
#include <iostream>

int main() {
  Lexer lex;
  // auto tokens = lex.lexer("VAR x = 5 AS INTEGER; IF x > 3 THEN RETURN x;
  // END;");

  auto tokens = lex.lexer("PRINT(1)");
  for (auto &tok : tokens)
    std::cout << tok[0] << " : " << tok[1] << "\n";

  std::cout << Colors::BOLD << Colors::GREEN << "PARSED VALUE" << Colors::RESET
            << "\n";

  std::vector<std::vector<std::vector<std::string>>> fullcode;
  fullcode.push_back(tokens);

  Parser parse;

  std::vector<std::unique_ptr<ast>> parsed_output = parse.Parse(fullcode);
  for (auto &p : parsed_output) {
    std::cout << p->repr();
  }

  std::cout << "\n";
}
