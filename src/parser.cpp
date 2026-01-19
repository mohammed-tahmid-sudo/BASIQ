#include "ast.h"
#include "lexer.h"
#include <algorithm>
#include <colors.h>
#include <iostream>
#include <llvm-18/llvm/ADT/STLExtras.h>
#include <llvm-18/llvm/Support/AtomicOrdering.h>
#include <memory>
#include <parser.h>
#include <string>
#include <vector>

std::string Parser::Peek(bool i) {
  if (!i) {
    return tokenTypeToString(input[x][y].Type);
  } else if (i) {
    return input[x][y].Value;
  } else {
    std::cerr << "ERROR PEEK VALUE I IS TOO BIG";
  }

  return "";
}

void Parser::Consume() {
  if (x >= input.size())
    return;

  y++;
  if (y >= input[x].size()) {
    y = 0;
    x++;
  }
}

bool Parser::Expect(TokenType tok) {
  if (Peek() != tokenTypeToString(tok)) {
    std::cerr << "Expected " << tokenTypeToString(tok) << ", got " << Peek(true)
              << "\n";
    return false;
  }
  Consume();
  return true;
}

std::unique_ptr<ast> Parser::ParseExpressions() {
  std::unique_ptr<ast> output;

  if (Peek() == tokenTypeToString(TokenType::INTEGER_LITERAL) ||
      Peek() == tokenTypeToString(TokenType::FLOAT_LITERAL)) {

    output = std::make_unique<NumberNode>(std::stoi(Peek(true)));
    Consume();
  }

  if (Peek() == tokenTypeToString(TokenType::PLUS)) {

	  std::string op = Peek(true);
	  Consume();

	  auto right = ParseExpressions();

	  output = std::make_unique<BinaryOperationNode>(std::move(output), std::move(right), op[0]);
  }

  return output;
}

std::unique_ptr<ast> Parser::ParseStatements() {
  std::unique_ptr<ast> output;

  if (Peek() == tokenTypeToString(TokenType::IF)) {
    Consume();
    Expect(TokenType::LPAREN);
  }

  return output;
}

std::vector<std::unique_ptr<ast>> Parser::Parse() {

  std::vector<std::unique_ptr<ast>> output;

  output.push_back(ParseExpressions());
  output.push_back(ParseStatements());

  return output;
}

int main() {
  Lexer lex;
  std::vector<std::vector<Token>> statements =
      lex.lexerSplitStatements("1 + 1");

  for (size_t idx = 0; idx < statements.size(); ++idx) {
    for (const auto &tok : statements[idx]) {
      std::cout << "  " << tokenTypeToString(tok.Type) << " : " << tok.Value
                << "\n";
    }
    std::cout << Colors::BOLD << Colors::RED << "---------------------\n"
              << Colors::RESET;
  }
  std::cout << Colors::BOLD << Colors::GREEN << "---------------------"
            << "PARSER" << "---------------------\n"
            << Colors::RESET;
  Parser parse(statements);

  auto output = parse.Parse();

  for (auto &tok : output) {
    std::cout << tok->repr() << std::endl;
  }
}
