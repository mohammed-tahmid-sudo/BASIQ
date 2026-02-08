#include "lexer.h"
#include <ast.h>
#include <colors.h>
#include <iomanip>
#include <iostream>
#include <llvm-18/llvm/IR/Intrinsics.h>
#include <llvm-18/llvm/Support/AtomicOrdering.h>
#include <llvm-18/llvm/Support/CommandLine.h>
#include <memory>
#include <parser.h>
#include <stdexcept>
#include <vector>

Token Parser::Peek() const {

  if (x >= code.size()) {
    return Token{TokenType::EOF_TOKEN, ""};
  }
  if (y >= code[x].size()) {
    return Token{TokenType::EOF_TOKEN, ""};
  }
  return code[x][y];
}

Token Parser::Consume() {
  if (x >= code.size()) {
    return Token{TokenType::EOF_TOKEN, ""};
  }

  Token current = code[x][y];
  y++;

  if (y >= code[x].size()) {
    x++;
    y = 0;
  }

  return current;
}

Token Parser::Expect(TokenType expected) {
  Token tok = Peek();
  if (tok.type != expected) {
    throw std::runtime_error("unexpected token");
  }
  return Consume();
}

std::unique_ptr<VariableDeclareNode> Parser::ParserVariable() {
  Expect(TokenType::LET);
  Token val = Expect(TokenType::IDENTIFIER);
  std::string name = val.value;
  Expect(TokenType::COLON);

  TokenType Type;
  TokenType peekType = Peek().type;
  if (peekType == TokenType::INTEGER || peekType == TokenType::FLOAT ||
      peekType == TokenType::BOOLEAN || peekType == TokenType::STRING) {
    Type = peekType;
    Consume();
  } else {
    std::cerr << Colors::BOLD << Colors::RED << "ERROR:" << Colors::RESET
              << "Invalid type\n";
    return nullptr;
  }

  std::unique_ptr<ast> args;

  // optional initialization
  if (Peek().type == TokenType::EQ) {
    Consume();
    args = ParseExpression();
  }

  // **expect statement EOF token** (added by lexer)
  Expect(TokenType::EOF_TOKEN);

  // DO NOT consume again — Expect already advanced
  // Consume(); <-- remove this

  return std::make_unique<VariableDeclareNode>(name, std::move(args), Type);
}

std::unique_ptr<ast> Parser::ParseExpression() {
  if (auto v = ParserVariable()) {
    return v;
  } else {
    return nullptr;
  }
}



std::vector<std::unique_ptr<ast>> Parser::Parse() {
  std::vector<std::unique_ptr<ast>> nodes;

  while (true) {
    Token tok = Peek();
    if (tok.type == TokenType::EOF_TOKEN) {
      break; // Stop when we reach the end of all code
    }

    std::unique_ptr<ast> node = ParseExpression();
    if (node) {
      nodes.push_back(std::move(node));
    } else {
      // Skip token if parsing failed, to avoid infinite loop
      Consume();
    }
  }

  return nodes;
}

int main() {
  // std::string src = R"(
  // @version "1.0";
  // @author "Tahmid";

  // let x: Integer = 10;
  // const let y: Float = 3.14;

  // func add(a: Integer, b: Integer) -> void {
  // return a + b;
  // };

  // if x >= 5 {
  // print ("ok");
  // } else {
  // print ("no");
  // };

  // for i in 0..10 {
  // print (i);
  // };

  // )";

  std::string src = R"(
  let x:Integer;
  )";
  Lexer lexer(src);
  auto program = lexer.lexer();

  int stmtNo = 0;
  for (const auto &stmt : program) {
    std::cout << "Statement " << stmtNo++ << ":\n";
    for (const auto &tok : stmt) {
      std::cout << "  " << std::setw(12) << tokenName(tok.type) << " : '"
                << tok.value << "'\n";
    }
    std::cout << "\n";
  }
  std::cout
      << Colors::RED
      << "----------------------------------------------------------------"
      << Colors::RESET << std::endl;

  Parser parse(program);
  auto output = parse.Parse();

  for (auto &tok : output) {
    std::cout << tok->repr() << std::endl;
  }
}
