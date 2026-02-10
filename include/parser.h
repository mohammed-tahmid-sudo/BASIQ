#pragma once
#include "lexer.h"
#include <ast.h>
#include <memory>
#include <vector>

class Parser {
  std::vector<std::vector<Token>> code;
  size_t x = 0;
  size_t y = 0;
  CodegenContext cc;

public:
  Parser(const std::vector<std::vector<Token>> &tokens, std::string filename)
      : code(tokens), cc(filename) {}

  Token Peek() const;
  Token Consume();
  Token Expect(TokenType expected);

  std::unique_ptr<ast> ParseFactor();
  std::unique_ptr<ast> ParseTerm();
  std::unique_ptr<ast> ParseExpression();

  std::unique_ptr<VariableDeclareNode> ParserVariable();
  std::unique_ptr<FunctionNode> parseFunction();
  std::unique_ptr<CompoundNode> parseCompound();
  std::unique_ptr<ast> ParseStatements();

  std::vector<std::unique_ptr<ast>> Parse();
};
