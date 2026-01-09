#include <ast.h>
#include <colors.h>
#include <iostream>
#include <lexer.h>
#include <memory>
#include <parser.h>
#include <stdexcept>
#include <string>
#include <vector>

std::string Parser::Peek(int i) {
  if (i != 1 && i != 0)
    throw std::out_of_range("YOU'RE PEEKING VALUE IS OUR OF RANGE");

  return input[x][y][i];
}

void Parser::consume() {

  if (input[x][y][0] != "EOF") {
    y++;
  } else if (input[x][y][0] == "EOF") {
    x++;
  }
}

std::vector<std::unique_ptr<ast>> Parser::Parse() {
  std::vector<std::unique_ptr<ast>> output;

  if (Peek(0) == "VarKeyword") {
    std::string VarName;
    std::string Type;

    consume();

    if (Peek(0) == "Identifier") {
      VarName += Peek(1);
      consume();

      if (Peek(0) == "AsKeyword") {
        consume();
        if (Peek(0) == "Type") {
          Type += Peek(1);
          consume();
          if (Peek(0) == "Semicolon") {
            output.push_back(std::make_unique<VariableDeclareNode>(
                std::make_unique<VariableNode>(VarName), Type,
                std::vector<std::unique_ptr<ast>>{}));

          } else {
            std::cerr << "EXPECTED A SEMICOLON \";\" AT THE END OF THE "
                         "VARIABLE DECLERATION\n";
          }
        }
      } else if (Peek(0) == "Equal") {

        std::vector<std::vector<std::string>> stmt;
        std::string Type;

        while (Peek(0) != "Semicolon") {
          if (Peek(0) == "AsKeyword") {
            break;
          } else {
            stmt.push_back({Peek(0), Peek(1)});
            consume();
          }
        }

        if (Peek(0) != "AsKeyword") {
          std::cerr << "EXPECTED 'AS' KEYWORD AFTER VARIABLE DECLERATION\n";

        } else {
          consume();
          if (Peek(0) == "Type") {
            Type += Peek(1);
            consume();

            if (Peek(0) != "Semicolon") {
              std::cerr << "EXPECTED ';' AFTER DECLARING THE VARIABLE TYPE\n";
            }
          }
        }

        Parser parse({stmt});
        output.push_back(std::make_unique<VariableDeclareNode>(
            std::make_unique<VariableNode>(VarName), Type, parse.Parse()));
      }
    }
  } else if (Peek(0) == "Number") {
    output.push_back(std::make_unique<NumberNode>(std::stoi(Peek(1))));
    consume();
  }

  return output;
}
