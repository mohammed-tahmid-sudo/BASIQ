#include "lexer.h"
#include <ast.h>
#include <codegen.h>
#include <colors.h>
#include <iomanip>
#include <iostream>
#include <llvm-18/llvm/IR/Function.h>
#include <llvm-18/llvm/IR/Intrinsics.h>
#include <llvm-18/llvm/Support/AtomicOrdering.h>
#include <llvm-18/llvm/Support/CommandLine.h>
#include <memory>
#include <parser.h>
#include <stdexcept>
#include <string>
#include <utility>
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

  // If we've run past the tokens in the current statement,
  // advance to the next statement and return EOF_TOKEN.
  if (y >= code[x].size()) {
    x++;
    y = 0;
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

    throw std::runtime_error("Expected " + std::string(tokenName(expected)));
  }
  return Consume();
}

std::unique_ptr<ast> Parser::ParseFactor() {
  if (Peek().type == TokenType::INT_LITERAL) {
    int val = std::stoi(Peek().value);
    Consume();
    return std::make_unique<IntegerNode>(val);

  } else if (Peek().type == TokenType::FLOAT_LITERAL) {
    float val = std::stof(Peek().value);
    Consume();
    return std::make_unique<FloatNode>(val);

  } else if (Peek().type == TokenType::STRING_LITERAL) {
    std::string val = Peek().value;
    Consume();
    return std::make_unique<StringNode>(val);

  } else if (Peek().type == TokenType::TRUE) {
    Consume();
    return std::make_unique<BooleanNode>(1);

  } else if (Peek().type == TokenType::FALSE) {
    Consume();
    return std::make_unique<BooleanNode>(0);

  } else if (Peek().type == TokenType::LPAREN) {
    Consume();
    auto val = ParseExpression();
    if (Peek().type != TokenType::RPAREN)
      throw std::runtime_error("Expected ')'");
    Consume();
    return val;
  } else {
    return nullptr;
  }
}
std::unique_ptr<ast> Parser::ParseTerm() {
  std::unique_ptr<ast> left = ParseFactor();

  while (Peek().type == TokenType::STAR || Peek().type == TokenType::SLASH) {
    TokenType token = Peek().type;
    Consume();
    left = std::make_unique<BinaryOperationNode>(token, std::move(left),
                                                 ParseFactor());
  }

  return left;
}
std::unique_ptr<ast> Parser::ParseExpression() {
  auto left = ParseTerm();

  while (Peek().type == TokenType::PLUS || Peek().type == TokenType::MINUS) {
    TokenType token = Peek().type;
    Consume();
    left = std::make_unique<BinaryOperationNode>(token, std::move(left),
                                                 ParseTerm());
  }
  return left;
}

std::unique_ptr<VariableDeclareNode> Parser::ParserVariable() {
  Expect(TokenType::LET);
  Token val = Expect(TokenType::IDENTIFIER);
  std::string name = val.value;
  Expect(TokenType::COLON);

  Token Type = Expect(TokenType::Types);

  std::unique_ptr<ast> args;

  if (Peek().type == TokenType::EQ) {
    Consume();
    args = ParseExpression();
  }

  // Expect(TokenType::EOF_TOKEN);

  return std::make_unique<VariableDeclareNode>(name, std::move(args), Type);
}
std::unique_ptr<FunctionNode> Parser::parseFunction() {
  Expect(TokenType::FUNC);
  Token v = Expect(TokenType::IDENTIFIER);
  Expect(TokenType::LPAREN);

  std::vector<std::pair<std::string, llvm::Type *>> args;

  while (Peek().type != TokenType::RPAREN) {
    Token val = Expect(TokenType::IDENTIFIER);
    Expect(TokenType::COLON);
    Token type = Expect(TokenType::Types);

    args.push_back(
        std::make_pair(val.value, GetTypeVoid(type, *cc.TheContext)));
    Consume();
  }
  Consume();

  Expect(TokenType::DOT);
  Token type = Expect(TokenType::Types);

  std::unique_ptr<ast> node = ParseStatements();
  if (!node) {
    node = ParseExpression();
  }
  return std::make_unique<FunctionNode>(v.value, args, std::move(node), type);
}

std::unique_ptr<ast> Parser::ParseStatements() {
  if (Peek().type == TokenType::LET) {
    auto v = ParserVariable();
    return v;
  } else if (Peek().type == TokenType::FUNC) {
    auto v = parseFunction();
    return v;
  } else {
    return nullptr;
  }
}

std::vector<std::unique_ptr<ast>> Parser::Parse() {
  std::vector<std::unique_ptr<ast>> nodes;

  while (true) {
    Token tok = Peek();
    if (tok.type == TokenType::EOF_TOKEN)
      break;

    std::unique_ptr<ast> node = ParseStatements();

    if (!node) {
      ParseExpression();
    }

    if (node) {
      nodes.push_back(std::move(node));
    } else {
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
	func add() -> void 1 + 1;
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

  Parser parse(program, "MyProgram");
  auto output = parse.Parse();

  for (auto &tok : output) {
    std::cout << tok->repr() << std::endl;
  }
}
