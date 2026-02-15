#include <ast.h>
#include <lexer.h>
#include <memory>
#include <vector>
class Parser {
  std::vector<Token> input;
  size_t x = 0;
  CodegenContext cc;

public:
  Parser(std::vector<Token> inp, const std::string &s) : input(inp), cc(s) {}
  Token Peek();
  Token Consume();
  Token Expect(TokenType tk);

  std::unique_ptr<ast> ParseFactor();
  std::unique_ptr<ast> ParseTerm();

  std::unique_ptr<ast> ParseExpression();

  std::unique_ptr<VariableDeclareNode> ParseVariable();
  std::unique_ptr<FunctionNode> ParseFunction();
  std::unique_ptr<ast> ParseStatement();

  std::vector<std::unique_ptr<ast>> Parse();
};
