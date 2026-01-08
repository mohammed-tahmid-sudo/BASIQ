#include <ast.h>
#include <colors.h>
#include <cstdint>
#include <iostream>
#include <lexer.h>
#include <memory>
#include <parser.h>
#include <string>
#include <vector>

std::vector<std::unique_ptr<ast>>
Parser::Parse(std::vector<std::vector<std::vector<std::string>>> input)
{
  std::vector<std::unique_ptr<ast>> output;

  for (size_t x = 0; x < input.size(); ++x)
  {
    for (size_t y = 0; y < input[x].size(); ++y)
    {
      if (input[x][y][0] == "VarKeyword")
      {
        if (y + 1 < input[x].size() && input[x][y + 1][0] == "Identifier")
        {
          if (y + 2 < input[x].size() && input[x][y + 2][0] == "AsKeyword")
          {
            if (y + 3 < input[x].size() && input[x][y + 3][0] == "Type")
            {

              if (y + 4 < input[x].size() &&
                  input[x][y + 4][0] == "Semicolon")
              {
                output.emplace_back(std::make_unique<VariableDeclareNode>(
                    input[x][y + 1][1], input[x][y + 3][1]));
                y += 4;
              }
            }
          }
          else if (y + 2 < input[x].size() &&
                   input[x][y + 2][0] == "Assign")
          {

            size_t ASkeyLoc = y;
            while (y + 2 < input[x].size() &&
                   input[x][ASkeyLoc + 2][0] != "EOF" &&
                   input[x][ASkeyLoc + 2][0] != "AsKeyword")
            {
              ASkeyLoc++;
            }

            if (ASkeyLoc + 2 < input[x].size() &&
                input[x][ASkeyLoc + 2][0] == "EOF")
            {
            }
            else if (ASkeyLoc + 2 < input[x].size() &&
                     input[x][ASkeyLoc + 2][0] == "AsKeyword")
            {

              if (ASkeyLoc + 3 < input[x].size() &&
                  input[x][ASkeyLoc + 3][0] == "Type")
              {

                if (ASkeyLoc + 4 < input[x].size() &&
                    input[x][ASkeyLoc + 4][0] == "Semicolon")
                {

                  auto value = std::vector<std::vector<std::string>>(
                      input[x].begin() + y + 3,
                      input[x].begin() + ASkeyLoc + 2);

                  auto value_parsed = Parse({value});

                  output.emplace_back(std::make_unique<AssignmentNode>(
                      std::make_unique<VariableNode>(input[x][y + 1][1]),
                      std::move(value_parsed), input[x][ASkeyLoc + 3][1]));
                  y = ASkeyLoc + 4;
                }
              }
            }
          }
        }
      }
      else if (input[x][y][0] == "Number")
      {
        output.emplace_back(
            std::make_unique<NumberNode>(std::stoi(input[x][y][1])));
      }
      else if (input[x][y][0] == "PrintKeyword")
      {

        if (y < input[x].size() && input[x][y + 1][0] == "LParen")
        {
          size_t RParLoc = y + 1;
          while (RParLoc < input[x].size() && input[x][RParLoc][0] != "EOF")
          {

            if (RParLoc < input[x].size() && input[x][RParLoc][0] == "RParen")
            {
              break;
            }

            RParLoc++;
          }

          auto expr = std::vector<std::vector<std::string>>(
              input[x].begin() + y + 1, input[x].begin() + RParLoc);

          y = RParLoc;

          std::vector<std::unique_ptr<ast>> expr_parsed = Parse({expr});

          if (expr_parsed.empty())
          {
            std::cerr << "EMPTY PRINT STATEMENT\n";
            break;
          }

          output.emplace_back(
              std::make_unique<PrintNode>(std::move(expr_parsed[0])));
        }
      }
      else if (input[x][y][0] == "String")
      {
        output.emplace_back(std::make_unique<StringNode>(input[x][y][1]));
      }
      else if (input[x][y][0] == "WhileKeyword")
      {

        size_t count = y + 1;

        while (count < input[x].size() && input[x][count][0] != "EOF")
        {
          if (input[x][count][0] == "DoKeyword")
          {
            break;
          }
          count++;
        }

        auto condition = std::vector<std::vector<std::string>>(
            input[x].begin() + y + 1, input[x].begin() + count);

        y = count;
        // assume DoKeyword found at (startX,startY) where startX==x and startY==y
        size_t startX = x;
        size_t startY = y;
        int nesting = 1;
        bool found = false;
        size_t endX = startX, endY = startY + 1;

        // find matching EndKeyword
        for (size_t i = startX; i < input.size() && !found; ++i)
        {
          size_t j = (i == startX ? startY + 1 : 0);
          for (; j < input[i].size(); ++j)
          {
            if (input[i][j][0] == "DoKeyword")
              ++nesting;
            else if (input[i][j][0] == "EndKeyword")
            {
              --nesting;
              if (nesting == 0)
              {
                endX = i;
                endY = j;
                found = true;
                break;
              }
            }
          }
        }

        if (!found)
        {
          // handle error: unmatched Do
          throw std::runtime_error("Unmatched DoKeyword");
        }

        // collect tokens between (startX,startY) and (endX,endY), excluding Do and End
        std::vector<std::vector<std::string>> argument;
        for (size_t i = startX; i <= endX; ++i)
        {
          size_t sj = (i == startX ? startY + 1 : 0);
          size_t ej = (i == endX ? endY : input[i].size()); // stop before EndKeyword
          for (size_t j = sj; j < ej; ++j)
          {
            argument.push_back(input[i][j]); // each input[i][j] is vector<string>
          }
        }

        // if you want to resume parsing after the EndKeyword:
        x = endX;
        y = endY;
        auto parsed_condition = Parse({condition});
        if (parsed_condition.empty())
        {
          std::cerr << "ERROR PARSED CONDITION IS EMPTY\n";
        }

        auto parsed_argument = Parse({argument});
        if (parsed_argument.empty())
        {
          std::cerr << "ERROR PARSED argument IS EMPTY\n";
        }

        output.push_back(std::make_unique<ast>(WhileNode(std::move(parsed_condition[0]), std::move(parsed_argument))));
      }
    }
  }
  return output;
}
