
#pragma once
#include <lexer.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>
#include <unordered_map>

#include <memory>

enum Types { INTEGERTYPE, FLOATTYEP, BOOLEANTYPE, STRINGTYPE, IDENTIFIERTYPE };
enum BinaryOpTokentype {
  PLUSOP,
  MINUSOP,
  MULOP,
  DIVOP,
  EQUALOP,
  NOTEQUALOP,
  LESSTHANOP,
  GREATERTHANOP,
  LESSTHANEQUALOP,
  GREATERTHANEQUALOP,
  ANDOP,
  OROP
};

std::string TypesToString(Types tp);
std::string binaryOpTokenTypeToString(BinaryOpTokentype op);

struct CodegenContext {
  std::unique_ptr<llvm::LLVMContext> TheContext;
  std::unique_ptr<llvm::Module> Module;
  std::unique_ptr<llvm::IRBuilder<>> Builder;
  std::unordered_map<std::string, llvm::Value *> NamedValues;

  CodegenContext(const std::string &moduleName) {
    TheContext = std::make_unique<llvm::LLVMContext>();
    Module = std::make_unique<llvm::Module>(moduleName, *TheContext);
    Builder = std::make_unique<llvm::IRBuilder<>>(*TheContext);
  }
};

struct ast {
  virtual std::string repr() = 0;
  virtual llvm::Value *codegen(CodegenContext &cc) = 0;
  virtual ~ast() = default;
};

struct IntLiteralNode : ast {
  int val = 0;

  IntLiteralNode(const int v) : val(v) {}

  virtual std::string repr();
  virtual llvm::Value *codegen(CodegenContext &cc);
};
struct FloatLiteralNode : ast {
  float val = 0;

  FloatLiteralNode(const float v) : val(v) {}

  virtual std::string repr();
  virtual llvm::Value *codegen(CodegenContext &cc);
};

struct StringLiteralNode : ast {
  std::string val = 0;

  StringLiteralNode(const std::string v) : val(v) {}

  virtual std::string repr();
  virtual llvm::Value *codegen(CodegenContext &cc);
};

struct BooleanLiteralNode : ast {
  bool val = 0;

  BooleanLiteralNode(const bool v) : val(v) {}

  virtual std::string repr();
  virtual llvm::Value *codegen(CodegenContext &cc);
};

struct BinaryOperationNode : ast {
  std::unique_ptr<ast> left;
  std::unique_ptr<ast> right;
  BinaryOpTokentype op;

  BinaryOperationNode(std::unique_ptr<ast> LHS, std::unique_ptr<ast> RHS)
      : left(std::move(LHS)), right(std::move(RHS)) {}

  virtual std::string repr();
  virtual llvm::Value *codegen(CodegenContext &cc);
};

struct VariableDeclareNode : ast {
  std::string name;
  std::unique_ptr<ast> Content;
  Types type;
  VariableDeclareNode(std::string nm, std::unique_ptr<ast> cntnt, Types tp)
      : name(nm), Content(std::move(cntnt)), type(tp) {}

  virtual std::string repr();
  virtual llvm::Value *codegen(CodegenContext &cc);
};

struct AssignmentNode : ast {
  std::string name;
  std::unique_ptr<ast> Content;
  Types type;
  AssignmentNode(std::string nm, std::unique_ptr<ast> cntnt, Types tp)
      : name(nm), Content(std::move(cntnt)), type(tp) {}

  virtual std::string repr();
  virtual llvm::Value *codegen(CodegenContext &cc);
};

struct FunctionNode : ast {
  std::string name;
  std::unique_ptr<ast> contents;
  FunctionNode(std::string nm, std::unique_ptr<ast> cntnt)
      : name(nm), contents(std::move(cntnt)) {}

  virtual std::string repr();
  virtual llvm::Value *codegen(CodegenContext &cc);
};

struct CompountNode : ast {
  std::vector<std::unique_ptr<ast>> contents;

  CompountNode(std::vector<std::unique_ptr<ast>> cntnt)
      : contents(std::move(cntnt)) {}
  virtual std::string repr();
  virtual llvm::Value *codegen(CodegenContext &cc);
};

struct IfNode : ast {
  std::unique_ptr<ast> args;
  std::unique_ptr<ast> block;
  std::unique_ptr<ast> elseBlock;

  IfNode(std::unique_ptr<ast> arg, std::unique_ptr<ast> blk,
         std::unique_ptr<ast> elseblk)
      : args(std::move(arg)), block(std::move(blk)),
        elseBlock(std::move(elseblk)) {}

  virtual std::string repr();
  virtual llvm::Value *codegen(CodegenContext &cc);
};

struct WhileNode : ast {
  std::unique_ptr<ast> args;
  std::unique_ptr<ast> block;

  WhileNode(std::unique_ptr<ast> arg, std::unique_ptr<ast> blk,
            std::unique_ptr<ast> elseblk)
      : args(std::move(arg)), block(std::move(blk)) {}

  virtual std::string repr();
  virtual llvm::Value *codegen(CodegenContext &cc);
};
