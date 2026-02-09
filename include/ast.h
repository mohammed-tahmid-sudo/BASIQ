#pragma once
#include "lexer.h"
#include <llvm-18/llvm/IR/IRBuilder.h>
#include <llvm-18/llvm/IR/LLVMContext.h>
#include <llvm-18/llvm/IR/Type.h>
#include <llvm-18/llvm/IR/Value.h>
#include <memory>
#include <string>
#include <vector>

struct CodegenContext {
  std::unique_ptr<llvm::LLVMContext> TheContext;
  std::unique_ptr<llvm::IRBuilder<>> Builder;
  std::unique_ptr<llvm::Module> Module;
  std::vector<std::unordered_map<std::string, llvm::Value *>> NamedValuesStack;

  llvm::BasicBlock *BreakBB = nullptr;
  llvm::BasicBlock *ContinueBB = nullptr;

  // Scopes
  void pushScope() { NamedValuesStack.push_back({}); }
  void popScope() { NamedValuesStack.pop_back(); }

  void addVariable(const std::string &name, llvm::Value *value) {
    NamedValuesStack.back()[name] = value;
  }

  llvm::Value *lookup(const std::string &name) {
    for (auto it = NamedValuesStack.rbegin(); it != NamedValuesStack.rend();
         ++it) {
      if (it->count(name))
        return (*it)[name];
    }
    return nullptr;
  }

  CodegenContext(const std::string &name)
      : TheContext(std::make_unique<llvm::LLVMContext>()),
        Builder(std::make_unique<llvm::IRBuilder<>>(*TheContext)),
        Module(std::make_unique<llvm::Module>(name, *TheContext)) {}
};

// struct CodegenContext {
//   std::unique_ptr<llvm::LLVMContext> TheContext;
//   std::unique_ptr<llvm::IRBuilder<>> Builder;
//   std::unique_ptr<llvm::Module> Module;
//   std::vector<std::unordered_map<std::string, llvm::Value *>>
//   NamedValuesStack;

//   // Enter a new scope
//   void pushScope() { NamedValuesStack.push_back({}); }

//   // Exit current scope
//   void popScope() { NamedValuesStack.pop_back(); }

//   // Add a variable to the current scope
//   void addVariable(const std::string &name, llvm::Value *value) {
//     NamedValuesStack.back()[name] = value;
//   }

//   // Lookup variable (from innermost to outermost scope)
//   llvm::Value *lookup(const std::string &name) {
//     for (auto it = NamedValuesStack.rbegin(); it != NamedValuesStack.rend();
//          ++it) {
//       if (it->count(name))
//         return (*it)[name];
//     }
//     return nullptr; // not found
//   }

//   CodegenContext(const std::string &name)
//       : TheContext(std::make_unique<llvm::LLVMContext>()),
//         Builder(std::make_unique<llvm::IRBuilder<>>(*TheContext)),
//         Module(std::make_unique<llvm::Module>(name, *TheContext)) {}
// };

struct ast {
  virtual ~ast() = default;
  virtual std::string repr() = 0;
  virtual llvm::Value *codegen(CodegenContext &cc) = 0;
};

struct IntegerNode : ast {
  int val;
  IntegerNode(const int v) : val(v) {}
  std::string repr() override;
  llvm::Value *codegen(CodegenContext &cc) override;
};

struct FloatNode : ast {
  float val;
  FloatNode(float v) : val(v) {}
  std::string repr() override;
  llvm::Value *codegen(CodegenContext &cc) override;
};

struct BooleanNode : ast {
  bool val;
  BooleanNode(bool v) : val(v) {}
  std::string repr() override;
  llvm::Value *codegen(CodegenContext &cc) override;
};

struct StringNode : ast {
  std::string val;
  StringNode(const std::string &v) : val(v) {}
  std::string repr() override;
  llvm::Value *codegen(CodegenContext &cc) override;
};

struct VariableDeclareNode : ast {
  std::string name;
  Token Type;
  std::unique_ptr<ast> val;
  VariableDeclareNode(const std::string &n, std::unique_ptr<ast> v, Token t)
      : name(n), val(std::move(v)), Type(t) {}
  std::string repr() override;
  llvm::Value *codegen(CodegenContext &cc) override;
};

struct AssignmentNode : ast {
  std::string name;
  std::unique_ptr<ast> val;
  AssignmentNode(const std::string &n, std::unique_ptr<ast> v)
      : name(n), val(std::move(v)) {}
  std::string repr() override;
  llvm::Value *codegen(CodegenContext &cc) override;
};

struct ReturnNode : ast {
  std::unique_ptr<ast> expr;
  ReturnNode(std::unique_ptr<ast> exp) : expr(std::move(exp)) {}
  std::string repr() override;
  llvm::Value *codegen(CodegenContext &cc) override;
};
struct CompoundNode : ast {
  std::vector<std::unique_ptr<ast>> blocks;
  CompoundNode(std::vector<std::unique_ptr<ast>> b) : blocks(std::move(b)) {}
  std::string repr() override;
  llvm::Value *codegen(CodegenContext &cc) override;
};

struct FunctionNode : ast {
  std::string name;
  std::vector<std::pair<std::string, llvm::Type *>> args;
  std::unique_ptr<ast> content;
  Token ReturnType;

  FunctionNode(const std::string &s,
               std::vector<std::pair<std::string, llvm::Type *>> ars,
               std::unique_ptr<ast> cntnt, Token RetType)
      : name(s), args(ars), content(std::move(cntnt)), ReturnType(RetType) {}

  std::string repr() override;
  llvm::Value *codegen(CodegenContext &cc) override;
};

struct VariableReferenceNode : ast {
  std::string Name;

  VariableReferenceNode(const std::string &s) : Name(s) {}
  std::string repr() override;
  llvm::Value *codegen(CodegenContext &cc) override;
};

struct WhileNode : ast {
  std::unique_ptr<ast> condition;
  std::unique_ptr<ast> body;

  WhileNode(std::unique_ptr<ast> condtn, std::unique_ptr<ast> bdy)
      : condition(std::move(condtn)), body(std::move(bdy)) {}
  std::string repr() override;
  llvm::Value *codegen(CodegenContext &cc) override;
};

struct IfNode : ast {
  std::unique_ptr<ast> condition;
  std::unique_ptr<ast> thenBlock;
  std::unique_ptr<ast> elseBlock;

  IfNode(std::unique_ptr<ast> cond, std::unique_ptr<ast> thenB,
         std::unique_ptr<ast> elseB)
      : condition(std::move(cond)), thenBlock(std::move(thenB)),
        elseBlock(std::move(elseB)) {}

  std::string repr() override;
  llvm::Value *codegen(CodegenContext &cc) override;
};

struct BinaryOperationNode : ast {
  std::unique_ptr<ast> Left;
  std::unique_ptr<ast> Right;
  TokenType Type;
  BinaryOperationNode(TokenType tp, std::unique_ptr<ast> LHS,
                      std::unique_ptr<ast> RHS)
      : Type(tp), Left(std::move(LHS)), Right(std::move(RHS)) {}

  std::string repr() override;
  llvm::Value *codegen(CodegenContext &cc) override;
};

struct BreakNode : ast {
  std::string repr() override;
  llvm::Value *codegen(CodegenContext &cc) override;
};

struct ContinueNode : ast {
  std::string repr() override;
  llvm::Value *codegen(CodegenContext &cc) override;
};
struct CallNode : ast {
  std::string name;
  std::vector<std::unique_ptr<ast>> args;

  CallNode(const std::string &s, std::vector<std::unique_ptr<ast>> arg)
      : name(s), args(std::move(arg)) {}
  std::string repr() override;
  llvm::Value *codegen(CodegenContext &cc) override;
};
