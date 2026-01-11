
#include "parser.h"
#include "ast.h"
#include <cmath>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

std::vector<std::unique_ptr<ast>>
Parser::Parse(std::vector<std::vector<std::vector<std::string>>> input,
              bool parse_stmt) {

  std::vector<std::unique_ptr<ast>> output;

  auto Peek = [&](int i) -> std::string {
    if (i != 0 && i != 1)
      std::cerr << "ERROR TRYING TO ACCESS OUTSIDE VALUE\n";
    if (x >= input.size() || y >= input[x].size() || i >= input[x][y].size()) {
      std::cerr << "OUT OF BOUNDS\n";
      return "EOF"; // safe fallback
    }
    return input[x][y][i];
  };

  auto consume = [&]() {
    if (input[x][y][0] != "EOF") {
      y++;
      if (y >= input[x].size()) {
        y = 0;
        x++;
      }
    } else {
      x++;
      y = 0;
    }
  };

  if (Peek(0) == "Number") {
    std::string number;
    number = Peek(1);
    consume();

    output.push_back(std::make_unique<NumberNode>(std::stoi(number)));
  }
  if (Peek(0) == "IfKeyword") {

    auto ParseIfAndElifStatement = [&]() {
      std::vector<std::vector<std::string>> args;
      std::vector<std::vector<std::string>> block;

      consume(); // consume if or elif keyword
      if (Peek(0) == "LParen") {
        while (Peek(0) != "RParen") {
          consume();
          args.push_back({Peek(0), Peek(1)});
        }

        if (Peek(0) == "RParen") {
          args.pop_back();
          consume();

          if (Peek(0) == "ThenKeyword") {
            size_t counter = 0;
            while (true) {

              if (Peek(0) == "ThenKeyword") {
                ++counter;
                consume();
              } else if (Peek(0) == "EndKeyword") {
                --counter;
                consume();
              } else {
                block.push_back({Peek(0), Peek(1)});
                consume();
              }

              if (counter == 0) {
                break;
              }
            }
          }
        }
      }
      return std::tuple(args, block);
    };

    auto [args, block] = ParseIfAndElifStatement();
    bool ElifBlockFound = false;

    // while (true) {
    //   if (Peek(0) == "ElifKeyword") {
    //     auto [Elif_args, Elif_block] = ParseIfAndElifStatement();
    //     ElifBlockFound = true;

    //   } else if (Peek(0) == "ElseKeyword") {

    //     auto [Else_args, Else_block] = ParseIfAndElifStatement();
    //     break;

    //   } else {
    //     break;
    //   }
    // }
    if (Peek(0) == "ElseKeyword") {
      std::vector<std::vector<std::string>> else_block;
      size_t counter = 0;

      consume();
      while (true) {

        if (Peek(0) == "ThenKeyword") {
          ++counter;
          consume();
        } else if (Peek(0) == "EndKeyword") {
          --counter;
          consume();
        } else {
          else_block.push_back({Peek(0), Peek(1)});
          consume();
        }

        if (counter == 0) {
          break;
        }
      }

      auto args_parsed = Parse({args}, false);
      auto block_parsed = Parse({block});
      auto else_block_parsed = Parse({else_block});

      output.push_back(std::make_unique<IfNode>(std::move(args_parsed),
                                                std::move(block_parsed),
                                                std::move(else_block_parsed)));

    } else {
      auto args_parsed = Parse({args}, false);
      auto block_parsed = Parse({block});
      output.push_back(std::make_unique<IfNode>(
          std::move(args_parsed), std::move(block_parsed),
          std::vector<std::unique_ptr<ast>>{}

          ));
    }
  }

  return output;
};
