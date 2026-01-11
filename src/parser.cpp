
#include "parser.h"
#include "ast.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <type_traits>

std::vector<std::unique_ptr<ast>>
Parser::Parse(std::vector<std::vector<std::vector<std::string>>> input,
              bool parse_stmt) {

  std::vector<std::unique_ptr<ast>> output;

  auto Peek = [&](int i) {
    if (i != 1 && i != 0) {
      std::cerr << "ERROR TRYING TO ACCESS OUTSIDE VALUE\n";
    }

    return input[x][y][i];
  };

  auto consume = [&]() {
    if (input[x][y][0] != "EOF")
      y++;
    else
      x++;
  };

  if (Peek(0) == "Number") {
    std::string number;
    number = Peek(1);
    consume();

    output.push_back(std::make_unique<NumberNode>(std::stoi(number)));
  }

  if (Peek(0) == "IfKeyword" && parse_stmt) {
    std::vector<std::vector<std::string>> args;
    size_t endcounter = 0;
    std::vector<std::vector<std::string>> true_args;

    consume();
    if (Peek(0) == "LParen") {
      consume();

      while (Peek(0) != "RParen") {
        args.push_back({Peek(0), Peek(1)});
        consume();
      }
    }
    if (args.empty()) {
      std::cerr << "EMPTY ARGUMENT";
    }

    if (Peek(0) == "RParen") {
      consume();

      if (Peek(0) == "ThenKeyword") {
        while (true) {

          if (Peek(0) == "EndKeyword") {
            --endcounter;
          } else if (Peek(0) == "StartKeyword") {
            ++endcounter;
          } else {
            true_args.push_back({Peek(0), Peek(1)});
            consume();

          }

        }
      }
    }

    for (auto &tok : true_args) {
      std::cout << tok[0] << " : " << tok[1] << std::endl;
    }
  }

  return output;
};
