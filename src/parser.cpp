#include <ast.h>
#include <colors.h>
#include <lexer.h>
#include <memory>
#include <parser.h>
#include <string>
#include <vector>

std::vector<std::unique_ptr<ast>>
Parser::Parse(std::vector<std::vector<std::vector<std::string>>> input) {
  std::vector<std::unique_ptr<ast>> output;

  for (size_t x = 0; x < input.size(); ++x) {
    for (size_t y = 0; y < input[x].size(); ++y) {
      if (input[x][y][0] == "VarKeyword") {
        if (y + 1 < input[x].size() && input[x][y + 1][0] == "Identifier") {
          if (y + 2 < input[x].size() && input[x][y + 2][0] == "AsKeyword") {
            if (y + 3 < input[x].size() && input[x][y + 3][0] == "Type") {

              if (y + 4 < input[x].size() &&
                  input[x][y + 4][0] == "Semicolon") {
                output.emplace_back(std::make_unique<VariableDeclareNode>(
                    input[x][y + 1][1], input[x][y + 3][1]));
                y += 4;
              }
            }
          } else if (y + 2 < input[x].size() &&
                     input[x][y + 2][0] == "Assign") {

            size_t ASkeyLoc = y;
            while (y + 2 < input[x].size() &&
                   input[x][ASkeyLoc + 2][0] != "EOF" &&
                   input[x][ASkeyLoc + 2][0] != "AsKeyword") {
              ASkeyLoc++;
            }

            if (ASkeyLoc + 2 < input[x].size() &&
                input[x][ASkeyLoc + 2][0] == "EOF") {

              std::cout << Colors::BOLD << Colors::RED
                        << "ERROR:" << Colors::RESET
                        << "Expected 'As' as keyword after VariableDeclaration";
            } else if (ASkeyLoc + 2 < input[x].size() &&
                       input[x][ASkeyLoc + 2][0] == "AsKeyword") {

              if (ASkeyLoc + 3 < input[x].size() &&
                  input[x][ASkeyLoc + 3][0] == "Type") {

                if (ASkeyLoc + 4 < input[x].size() &&
                    input[x][ASkeyLoc + 4][0] == "Semicolon") {

                  auto value = std::vector<std::vector<std::string>>(
                      input[x].begin() + y + 3,
                      input[x].begin() + ASkeyLoc + 2);

                  for (auto &tok : value) {
                    for (auto &tk : tok) {

                      std::cout << Colors::RED << tk << Colors::RESET
                                << std::endl;
                    }
                  }

                  auto value_parsed = Parse({value});

                  output.emplace_back(std::make_unique<AssignmentNode>(
                      std::make_unique<VariableNode>(input[x][y + 1][1],
                                                     input[x][ASkeyLoc + 3][1]),
                      std::move(value_parsed), input[x][ASkeyLoc + 3][1]));
                  y += ASkeyLoc + 4;
                }
              }
            }
          }
        }
      } else if (input[x][y][0] == "Number") {
        output.emplace_back(
            std::make_unique<NumberNode>(std::stoi(input[x][y][1])));
      } else if (input[x][y][0] == "PrintKeyword") {

        if (y < input[x].size() && input[x][y + 1][0] == "LParen") {
          size_t RParLoc = y + 1;
          while (RParLoc < input[x].size() && input[x][RParLoc][0] != "EOF") {

            if (RParLoc < input[x].size() && input[x][RParLoc][0] == "RParen") {
              break;
            }

            RParLoc++;
          }

          auto expr = std::vector<std::vector<std::string>>(
              input[x].begin() + y + 1, input[x].begin() + RParLoc);

          y = RParLoc;

          std::vector<std::unique_ptr<ast>> expr_parsed = Parse({expr});

          output.emplace_back(
              std::make_unique<PrintNode>(std::move(expr_parsed[0])));
        }
      }
    }
  }
  return output;
}

