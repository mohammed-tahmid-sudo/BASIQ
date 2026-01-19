#include <cctype>
#include <iostream>
#include <lexer.h>
#include <string>
#include <vector>


enum TokenType {};

class Lexer {
	size_t i = 0;
public:
  std::vector<std::vector<std::string>> lexer(const std::string &input) {
    std::vector<std::vector<std::string>> output;
	output.push_back({"SDFSDFSDF", "#$#$#$#$#"});
    return output;
  }
};

int main(int argc, char *argv[]) {
  Lexer lex;
  auto lexed_output = lex.lexer(
      "VAR A  = 3 AS INTEGER; IF A < SOMETHING THEN DO SOME PIECE OF SHIT");

  for (auto SOMETHING : lexed_output) {
	std::cout << SOMETHING[0]  << " : " << SOMETHING[1] << std::endl;
  }
  return 0;
}
