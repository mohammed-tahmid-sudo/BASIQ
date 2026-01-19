
#include "parser.h"
#include "ast.h"
#include <cmath>
#include <memory>
#include <lexer.h>
#include <string>
#include <utility>
#include <vector>

std::vector<std::unique_ptr<ast>>
Parser::Parse(std::vector<std::vector<std::vector<std::string>>> input,
              bool parse_stmt) {
  size_t x = 0;
  size_t y = 0;

  std::vector<std::unique_ptr<ast>> output;

  auto Peek = [&](int i) -> std::string {
    if (x >= input.size() || y >= input[x].size())
      return "EOF";
    if (i >= input[x][y].size())
      return "EOF";
    return input[x][y][i];
  };

  auto consume = [&]() {
    if (x >= input.size())
      return;

    if (y + 1 < input[x].size() - 1) {
      y++;
    } else {
      x++;
      y = 0;
    }
  };
  if (Peek(0) == "Number") {
    std::string number = Peek(1);

    consume();
    if (!number.empty())
      output.push_back(std::make_unique<NumberNode>(std::stoi(number)));
  }

  if (Peek(0) == "IfKeyword") {

    auto ParseIfAndElifStatement = [&]() {
      std::vector<std::vector<std::string>> args;
      std::vector<std::vector<std::string>> block;

      consume(); // consume if or elif keyword

      if (Peek(0) == "LParen") {
        consume();
        while (Peek(0) != "RParen") {
          args.push_back({Peek(0), Peek(1)});
          consume();
        }

        if (Peek(0) == "RParen") {
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
    auto args_parsed = Parse({args});
    auto block_parsed = Parse({block});

    auto ifNode = std::make_unique<IfNode>(std::move(args_parsed[0]),
                                           std::move(block_parsed));

    IfNode *currentIf = ifNode.get();

    while (Peek(0) == "ElifKeyword") {
      auto [Elif_args, Elif_block] = ParseIfAndElifStatement();
      auto Elif_args_parsed = Parse({Elif_args});
      auto Elif_block_parsed = Parse({Elif_block});

      // Create a new IfNode for this elif
      auto elifNode = std::make_unique<IfNode>(std::move(Elif_args_parsed[0]),
                                               std::move(Elif_block_parsed));

      // Append it to the elseBody of the current if/elif
      currentIf->elseBody.push_back(std::move(elifNode));

      // Move the pointer to the newly added elif for the next iteration
      currentIf = static_cast<IfNode *>(currentIf->elseBody.back().get());
    }

    if (Peek(0) == "ElseKeyword") {

      consume();
      std::vector<std::vector<std::string>> block;
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

        auto nodes = Parse({block});
        for (auto &n : nodes)
          currentIf->elseBody.push_back(std::move(n));
      }
    }

    output.push_back(std::move(ifNode));
  }
  return output;
};


