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
  } else if (Peek().type == TokenType::IDENTIFIER) {
    std::string val = Peek().value;
    Consume();
    // inside ParseFactor, replace the call-handling block with:
    if (Peek().type == TokenType::LPAREN) {
      Consume(); // consume '('
      std::vector<std::unique_ptr<ast>> inputs;

      if (Peek().type != TokenType::RPAREN) {
        std::cout << "COMMING HERE" << std::endl;
        while (true) {
          inputs.push_back(ParseExpression());
          if (Peek().type == TokenType::COMMA) {
            Consume(); // consume comma and continue
          } else {
            break;
          }
        }
      }

      Expect(TokenType::RPAREN); // require ')'
      // DO NOT expect a semicolon here — caller handles that.
      return std::make_unique<CallNode>(val, std::move(inputs));
    }

    return std::make_unique<VariableReferenceNode>(val);
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

  Expect(TokenType::SEMICOLON);

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
  if (!node)
    node = ParseExpression();

  return std::make_unique<FunctionNode>(v.value, args, std::move(node), type);
}

std::unique_ptr<CompoundNode> Parser::ParseCompound() {
  Expect(TokenType::LBRACE);

  std::vector<std::unique_ptr<ast>> vals;
  while (Peek().type != TokenType::RBRACE) {
    std::unique_ptr<ast> val = ParseStatements();
    if (!val) {
      Consume();
	  break;
    }

    vals.push_back(std::move(val));
    // Consume();
    // std::cout << tokenName(Peek().type) << std::endl;
  }
  Consume();

  return std::make_unique<CompoundNode>(std::move(vals));
}
std::unique_ptr<IfNode> Parser::ParseIfElse() {
  Expect(TokenType::IF);
  Expect(TokenType::LPAREN);
  std::unique_ptr<ast> args = ParseExpression();
  Expect(TokenType::RPAREN);
  std::unique_ptr<ast> block = ParseStatements();
  if (!block) {
    block = ParseExpression();
  }
  Expect(TokenType::ELSE);
  std::unique_ptr<ast> Elseblock = ParseStatements();
  if (!Elseblock)
    Elseblock = ParseExpression();

  return std::make_unique<IfNode>(std::move(args), std::move(block),
                                  std::move(Elseblock));
}
std::unique_ptr<ReturnNode> Parser::ParseReturn() {
  Expect(TokenType::RETURN);
  std::unique_ptr<ast> val = ParseExpression();

  return std::make_unique<ReturnNode>(std::move(val));
}

std::unique_ptr<AssignmentNode> Parser::ParseAssinment() {
  auto name = Expect(TokenType::IDENTIFIER);
  if (Peek().type == TokenType::LPAREN) {
    return nullptr;
  }
  Expect(TokenType::EQ);
  auto val = ParseExpression();
  Expect(TokenType::SEMICOLON);

  return std::make_unique<AssignmentNode>(name.value, std::move(val));
}

// std::unique_ptr<CallNode> Parser::ParseCall() {
//   Token name = Expect(TokenType::IDENTIFIER);
//   Expect(TokenType::LPAREN);
//   std::vector<std::unique_ptr<ast>> inputs;
//   while (Peek().type == TokenType::RPAREN) {
//     std::unique_ptr<ast> input = ParseExpression();
//     Expect(TokenType::COMMA);
//   }
//   Consume();
//   Expect(TokenType::SEMICOLON);

//   return std::make_unique<CallNode>(name.value, std::move(inputs));
// }

std::unique_ptr<ast> Parser::ParseStatements() {
  // std::cout << tokenName(Peek().type) << std::endl;
  if (Peek().type == TokenType::LET) {
    return ParserVariable();

  } else if (Peek().type == TokenType::FUNC) {
    return parseFunction();

  } else if (Peek().type == TokenType::LBRACE) {
    return ParseCompound();

  } else if (Peek().type == TokenType::IF) {
    return ParseIfElse();

  } else if (Peek().type == TokenType::RETURN) {
    return ParseReturn();

  } else if (Peek().type == TokenType::IDENTIFIER) {
    // std::unique_ptr<ast> v = ParseAssinment();
    // if (!v) {
    //   v = ParseCall();
    // }
    return ParseAssinment();

  } else {
    return ParseExpression();
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
      node = ParseExpression();
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
  func add(a:Integer) -> Integer {
	if (a + 1) {
		a = 21;
		return a; 
	} else {
		a = 31; 
		return a;  
	}
	return a; 
  }

  func main() -> Integer {
	let a:Integer = 32; 

	return add(a); 
  }
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

  // for (auto &tok : output) {
  //   std::cout << tok->repr() << std::endl;
  // }
  CodegenContext cc("my_program");
  for (auto &tok : output) {
    tok->codegen(cc);
  }

  cc.Module->print(llvm::outs(), nullptr);
}
