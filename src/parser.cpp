// parser.cpp
#include "parser.h"
#include "ast.h"
#include <cstdint>
#include <iostream>
#include <memory>
#include <utility>

std::string Parser::Peek(int i) { return input[x][y][i]; }

void Parser::consume() {
  if (input[x][y][0] != "EOF")
    y++;
  else
    x++;
}

void Parser::expect(const std::string &t) {
  if (Peek(0) != t)
    throw std::runtime_error("unexpected token");
  consume();
}

std::vector<std::unique_ptr<ast>> Parser::Parse() {
  std::vector<std::unique_ptr<ast>> out;

  while (Peek(0) != "EOF") {
    if (auto n = parseExpr(input))
      out.push_back(std::move(n));
    else if (auto n = parseStmt(input))
      out.push_back(std::move(n));
    else
      throw std::runtime_error("parse error");
  }

  return out;
}

std::unique_ptr<ast>
Parser::parseStmt(std::vector<std::vector<std::vector<std::string>>> input) {
  std::unique_ptr<ast> output;
  if (Peek(0) == "IfKeyword") {
    std::vector<std::vector<std::string>> args;

    std::vector<std::vector<std::string>> true_block;
    size_t counter = SIZE_MAX;

    consume();

    if (Peek(0) == "LParen") {

      while (Peek(0) != "RParen") {

        args.push_back({Peek(0), Peek(1)});
        consume();
      }

      if (Peek(0) == "RParen") {
        consume();
        if (Peek(0) == "ThenKeyword") {

          while (counter != 0) {
            if (Peek(0) == "EndKeyword") {
              --counter;
              consume();
            } else if (Peek(0) == "ThenKeyword") {
              ++counter; // I heard using ++ first and then writing the var is
                         // faster
              consume();
            } else {
              true_block.push_back({Peek(0), Peek(1)});
            }

            if (counter == 0)
              break; // JUST IN CASE
          }
          if (Peek(0) == "EndKeyword") {
            consume();
          }
          for (auto &tok : true_block) {
            std::cout << tok[0] << " : " << tok[1] << std::endl;
          }
        }
      }
    }
  }
  return output;
}

std::unique_ptr<ast>
Parser::parseExpr(std::vector<std::vector<std::vector<std::string>>> input) {

  if (Peek(0) == "VarKeyword") {
    std::string name;
    std::string Type;

    consume();

    if (Peek(0) == "Identifier") {
      consume();

      if (Peek(0) == "AsKeyword") {
        consume();
        if (Peek(0) == "Type") {
          Type = Peek(1);
          consume();
          if (Peek(0) == "Semicolon") {
            consume();
            auto var = std::make_unique<VariableNode>(name);

            return std::make_unique<VariableDeclareNode>(
                std::move(var), // must move
                Type, std::vector<std::unique_ptr<ast>>{});
          }
        }
      }
    }
  }
  return nullptr;
}
