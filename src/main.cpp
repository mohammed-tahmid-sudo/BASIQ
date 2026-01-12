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

  // fullcode.push_back(lex.lexer("VAR a AS INTEGER;"));
  // fullcode.push_back(lex.lexer("IF (1) THEN 2 END ELIF (3) THEN 34 END ELIF "
  //                              "(34) THEN 3234234 END ELSE THEN 324234 END
  //                              "));
  fullcode.push_back(lex.lexer("IF "));
  fullcode.push_back(lex.lexer("(1)"));
  fullcode.push_back(lex.lexer("THEN"));
  fullcode.push_back(lex.lexer("12323"));
  fullcode.push_back(lex.lexer("END"));
  fullcode.push_back(lex.lexer("ELSE"));
  fullcode.push_back(lex.lexer("THEN"));
  fullcode.push_back(lex.lexer("234"));
  fullcode.push_back(lex.lexer("END"));

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
