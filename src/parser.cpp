#include "lexer.h"
#include <ast.h>
#include <colors.h>
#include <functional>
#include <iostream>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <memory>
#include <ostream>
#include <parser.h>
#include <string>
#include <vector>
#include <map>


// Error helper
// llvm::Value *LogErrorV(const char *Str);

static std::unique_ptr<llvm::LLVMContext> TheContext;
static std::unique_ptr<llvm::IRBuilder<>> Builder;
static std::unique_ptr<llvm::Module> TheModule;
static std::map<std::string, llvm::Value *> NamedValues;

// actual definition and memory allocation
llvm::BasicBlock* CurrentLoopStart = nullptr;
llvm::BasicBlock* CurrentLoopEnd = nullptr;

// just declare it, no memory allocated yet
extern llvm::BasicBlock* CurrentLoopStart;
extern llvm::BasicBlock* CurrentLoopEnd;



std::string Parser::Peek(bool value) {
  if (x >= input.size() || y >= input[x].size())
    return "";

  if (!value) {
    return tokenTypeToString(input[x][y].Type);
  } else {
    return input[x][y].Value;
  }
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

std::unique_ptr<ast>
Parser::parseFuncExprFactor(std::function<std::unique_ptr<ast>()> func,
                            std::array<TokenType, 2> TokenType) {

  std::unique_ptr<ast> left = func();

  if (!left)
    return nullptr;

  while (Peek() == tokenTypeToString(TokenType[0]) ||
         Peek() == tokenTypeToString(TokenType[1])) {
    std::string t = Peek(true);
    Consume();
    std::unique_ptr<ast> right = func();
    if (!right)
      return nullptr;

    left = std::make_unique<BinaryOperationNode>(std::move(left),
                                                 std::move(right), t[0]);
  }

  return left;
}

std::unique_ptr<ast> Parser::ParseFactor() {

  if (Peek() == tokenTypeToString(TokenType::INTEGER_LITERAL) ||
      Peek() == tokenTypeToString(TokenType::FLOAT_LITERAL)) {

    std::string val = Peek(true);
    Consume();
    return std::make_unique<NumberNode>(std::stod(val));

  } else if (Peek() == tokenTypeToString(TokenType::LPAREN)) {

    Consume();
    auto node = ParseExpressions();
    Consume();
    return node;
  }

  return nullptr;
}

std::unique_ptr<ast> Parser::ParseTerm() {

  std::array<TokenType, 2> tokens = {TokenType::STAR, TokenType::SLASH};
  return parseFuncExprFactor([this]() { return ParseFactor(); }, tokens);
}
std::unique_ptr<ast> Parser::ParseBinOP() {

  std::array<TokenType, 2> tokens = {TokenType::PLUS, TokenType::MINUS};
  return parseFuncExprFactor([this]() { return ParseTerm(); }, tokens);
}

std::unique_ptr<StringNode> Parser::ParseStrings() {
  if (Peek() != tokenTypeToString(TokenType::STRING_LITERAL))
    return nullptr;

  std::string output = Peek(true);
  Consume();
  return std::make_unique<StringNode>(output);
}

std::unique_ptr<ast> Parser::ParseComparison() { return nullptr; }

std::unique_ptr<ast> Parser::ParseVariables() {

  if (Peek() != tokenTypeToString(TokenType::IDENTIFIER))
    return nullptr;

  std::string output = Peek(true);
  Consume();
  return std::make_unique<VariableNode>(output);
}

std::unique_ptr<ast> Parser::ParseExpressions() {

  auto saveX = x, saveY = y;

  if (auto node = ParseComparison())
    return node;

  if (auto node = ParseStrings())
    return node;

  if (auto node = ParseVariables())
    return node;

  x = saveX;
  y = saveY;
  return ParseBinOP();
}

std::unique_ptr<IfNode> Parser::ParseIfElseStatement() {
  std::unique_ptr<IfNode> output = nullptr;

  Expect(TokenType::LPAREN);
  std::unique_ptr<ast> condition = ParseExpressions();
  Expect(TokenType::RPAREN);

  Expect(TokenType::THEN);
  std::vector<std::unique_ptr<ast>> block = Parser::Parse();
  Expect(TokenType::END);

  output = std::make_unique<IfNode>(std::move(condition), std::move(block));

  auto current = output.get();

  std::cout << Peek() << std::endl;

  while (Peek() == tokenTypeToString(TokenType::ELIF)) {
    Consume();

    auto elifCondition = ParseExpressions();
    auto elifBody = Parser::Parse();
    auto elifNode =
        std::make_unique<IfNode>(std::move(elifCondition), std::move(elifBody));

    current->elseBody.push_back(std::move(elifNode));
    current = static_cast<IfNode *>(current->elseBody.back().get());
  }

  if (Peek() == tokenTypeToString(TokenType::ELSE)) {
    Consume();
    auto elseBlock = Parser::Parse();
    current->elseBody = std::move(elseBlock);
  }

  return output;
}

std::unique_ptr<WhileNode> Parser::ParseWhileStatement() {
  Expect(TokenType::LPAREN);
  std::unique_ptr<ast> args = ParseExpressions();
  Expect(TokenType::RPAREN);
  Expect(TokenType::DO);
  std::vector<std::unique_ptr<ast>> block = Parser::Parse();
  Expect(TokenType::END);

  return std::make_unique<WhileNode>(std::move(args), std::move(block));
}

std::unique_ptr<VariableDeclareNode> Parser::ParseVariableStatement() {

  std::unique_ptr<ast> args = nullptr;
  std::string name = Peek(true);
  std::string type;
  Expect(TokenType::IDENTIFIER);
  if (Peek() == tokenTypeToString(TokenType::EQUAL)) {
    Consume();
    args = ParseExpressions();
    Expect(TokenType::AS);
    type = Peek(true);
    Expect(TokenType::TYPE);

  } else if (Peek() == tokenTypeToString(TokenType::AS)) {
    Consume();
    type = Peek(true);
    Expect(TokenType::TYPE);
  }

  return std::make_unique<VariableDeclareNode>(name, type, std::move(args));
}

std::unique_ptr<ast> Parser::ParseStatements() {
  std::unique_ptr<ast> output;

  if (Peek() == tokenTypeToString(TokenType::IF)) {
    Consume();
    output = ParseIfElseStatement();

  } else if (Peek() == tokenTypeToString(TokenType::WHILE)) {
    Consume();
    output = ParseWhileStatement();
  } else if (Peek() == tokenTypeToString(TokenType::VAR)) {
    Consume();
    output = ParseVariableStatement();
  }

  return output;
}

std::vector<std::unique_ptr<ast>> Parser::Parse() {
  std::vector<std::unique_ptr<ast>> output;

  auto stmt = ParseStatements();
  if (stmt) {
    output.push_back(std::move(stmt));
  } else {
    auto expr = ParseExpressions();
    if (expr) {
      output.push_back(std::move(expr));
    }
  }

  return output;
}

// int main() {
//   Lexer lex;
//   std::vector<std::vector<Token>> statements = lex.lexerSplitStatements("a");

//   for (size_t idx = 0; idx < statements.size(); ++idx) {
//     for (const auto &tok : statements[idx]) {
//       std::cout << "  " << tokenTypeToString(tok.Type) << " : " << tok.Value
//                 << "\n";
//     }
//     std::cout << Colors::BOLD << Colors::RED << "---------------------\n"
//               << Colors::RESET;
//   }
//   std::cout << Colors::BOLD << Colors::GREEN << "---------------------"
//             << "PARSER" << "---------------------\n"
//             << Colors::RESET;
//   Parser parse(statements);

//   auto output = parse.Parse();

//   for (auto &tok : output) {
//     std::cout << tok->repr() << std::endl;
//   }
// }

int main() {
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();

  TheContext = std::make_unique<llvm::LLVMContext>();
  TheModule = std::make_unique<llvm::Module>("my_module", *TheContext);
  Builder = std::make_unique<llvm::IRBuilder<>>(*TheContext);

  // double main()
  auto *FT =
      llvm::FunctionType::get(llvm::Type::getDoubleTy(*TheContext), false);

  auto *F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, "main",
                                   TheModule.get());

  auto *BB = llvm::BasicBlock::Create(*TheContext, "entry", F);
  Builder->SetInsertPoint(BB);

  auto code = std::make_unique<BinaryOperationNode>(
      std::make_unique<NumberNode>(5), std::make_unique<NumberNode>(5), '+');

  llvm::Value *Result = code->codegen(std::move(TheContext), Builder, TheModule, NamedValues);
  Builder->CreateRet(Result);

  if (llvm::verifyFunction(*F, &llvm::errs())) {
    llvm::errs() << "Function verification failed\n";
    return 1;
  }

  // Print IR
  TheModule->print(llvm::outs(), nullptr);

  return 0;
}
