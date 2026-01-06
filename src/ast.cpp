#include "codegen.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include <llvm-18/llvm/IR/Constants.h>
#include <llvm-18/llvm/IR/IRBuilder.h>
#include <llvm-18/llvm/IR/Type.h>
#include <llvm-18/llvm/IR/Value.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

struct ast {
  virtual std::string repr() = 0;
  virtual llvm::Value *codegen() = 0;
  virtual ~ast() {}
};

// ------------------ Expression / Value Nodes ------------------

class NumberNode : public ast {
public:
  int number;
  NumberNode(int n) : number(n) {}
  llvm::Value *codegen() override {
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), number,
                                  true);
  }
  std::string repr() override {
    return "NumberNode(" + std::to_string(number) + ")";
  }
};

class VariableNode : public ast {
public:
  std::string name;
  std::string type;
  VariableNode(const std::string &n, const std::string tp)
      : name(n), type(tp) {}

  struct VariableDeclareNode : ast {
    std::string name;
    std::string type; // "int", "float", etc.

    llvm::Value *codegen() override {
      llvm::Type *llvmType = nullptr;
      if (type == "int")
        llvmType = llvm::Type::getInt32Ty(*TheContext);
      else if (type == "float")
        llvmType = llvm::Type::getFloatTy(*TheContext);
      else
        return nullptr; // unsupported type

      llvm::AllocaInst *Alloca =
          Builder->CreateAlloca(llvmType, nullptr, name);
      NamedValues[name] = Alloca;
      return Alloca;
    }
  };

  std::string repr() override { return "VariableNode(" + name + ")"; }
};

class VariableDeclareNode : public ast {
public:
  std::string name;
  std::string type;
  VariableDeclareNode(const std::string &n, const std::string &tp)
      : name(n), type(tp) {}

  llvm::Value *codegen() override {
    llvm::Type *llvmType = nullptr;
    if (type == "INT")
      llvmType = llvm::Type::getInt32Ty(*TheContext);

    else if (type == "FLOAT")
      llvmType = llvm::Type::getFloatTy(*TheContext);

    else if (type == "STRING")
      llvmType = llvm::Type::getInt8Ty(*TheContext); // pointer to char

    else {
      fprintf(stderr, "Unsupported variable type: %s\n", type.c_str());
      return nullptr;
    }
    // Allocate memory for the variable at the entry block of the current
    // function
    llvm::Function *func = Builder->GetInsertBlock()->getParent();
    llvm::IRBuilder<> TmpB(&func->getEntryBlock(),
                           func->getEntryBlock().begin());
    llvm::AllocaInst *Alloca = TmpB.CreateAlloca(llvmType, nullptr, name);

    NamedValues[name] = Alloca; // store pointer to the allocated variable

    return Alloca;
  }
  std::string repr() override {
    return "VariableDeclareNode(" + name + ", type=" + type + ")";
  }
};

class AssignmentNode : public ast {
public:
  const std::unique_ptr<VariableNode> name;
  std::vector<std::unique_ptr<ast>> value;
  std::string type;

  AssignmentNode(std::unique_ptr<VariableNode> n,
                 std::vector<std::unique_ptr<ast>> v, const std::string &t = "")
      : name(std::move(n)), value(std::move(v)), type(t) {}

  std::string repr() override {
    std::string s = "AssignmentNode(" + name->repr() + ", value=[";
    for (auto &v : value)
      s += v->repr() + ",";
    s += "], type=" + type + ")";
    return s;
  }
};

class BinaryOperationNode : public ast {
public:
  std::unique_ptr<ast> left;
  std::unique_ptr<ast> right;
  std::string op;
  BinaryOperationNode(std::unique_ptr<ast> l, std::unique_ptr<ast> r,
                      const std::string &o)
      : left(std::move(l)), right(std::move(r)), op(o) {}
  std::string repr() override {
    return "BinaryOperationNode(op=" + op + ", left=" + left->repr() +
           ", right=" + right->repr() + ")";
  }
};

class IdentifierNode : public ast {
public:
  std::string id;
  IdentifierNode(const std::string &n) : id(n) {}
  std::string repr() override { return "IdentifierNode(" + id + ")"; }
};

// ------------------ Control Flow Nodes ------------------

class ReturnNode : public ast {
public:
  std::unique_ptr<ast> expr; // optional
  ReturnNode(std::unique_ptr<ast> e = nullptr) : expr(std::move(e)) {}
  std::string repr() override {
    return "ReturnNode(" + (expr ? expr->repr() : "null") + ")";
  }
};

class BreakNode : public ast {
public:
  std::string repr() override { return "BreakNode"; }
};

class StringNode : public ast {
public:
  std::string value;
  StringNode(const std::string &v) : value(v) {}
  llvm::Value *codegen() override {
    // Create a global constant string
    return Builder->CreateGlobalStringPtr(value, "str");
  }

  std::string repr() override { return "StringNode(" + value + ")"; }
};
class ContinueNode : public ast {
public:
  std::string repr() override { return "ContinueNode"; }
};

class ComparisonNode : public ast {
public:
  std::vector<std::unique_ptr<ast>> left;
  std::vector<std::unique_ptr<ast>> right;
  std::string comp;

  ComparisonNode(std::vector<std::unique_ptr<ast>> l,
                 
                 std::vector<std::unique_ptr<ast>> r, const std::string &c)
      : left(std::move(l)), right(std::move(r)), comp(c) {}

  std::string repr() override {
    auto vec_repr = [](const std::vector<std::unique_ptr<ast>> &v) {
      std::string s = "[";
      for (size_t i = 0; i < v.size(); ++i) {
        s += v[i]->repr();
        if (i + 1 < v.size())
          s += ", ";
      }
      s += "]";
      return s;
    };

    return "ComparisonNode(comp=" + comp + ", left=" + vec_repr(left) +
           ", right=" + vec_repr(right) + ")";
  }
};

class IfNode : public ast {
public:
  std::vector<std::unique_ptr<ast>> condition;
  std::vector<std::unique_ptr<ast>> body;
  std::vector<std::unique_ptr<ast>> elseBody;

  IfNode(std::vector<std::unique_ptr<ast>> cond,
         std::vector<std::unique_ptr<ast>> IfNode,
         std::vector<std::unique_ptr<ast>> ElseNode = {})
      : condition(std::move(cond)), body(std::move(IfNode)),
        elseBody(std::move(ElseNode)) {}

  std::string repr() override {
    std::string s = "IfNode(cond=[";
    for (auto &n : condition)
      s += n->repr() + ",";
    if (!condition.empty())
      s.pop_back();

    s += "], body=[";
    for (auto &n : body)
      s += n->repr() + ",";
    if (!body.empty())
      s.pop_back();

    s += "]";
    if (!elseBody.empty()) {
      s += ", else=[";
      for (auto &n : elseBody)
        s += n->repr() + ",";
      s.pop_back();
      s += "]";
    }
    s += ")";
    return s;
  }
};

class WhileNode : public ast {
public:
  std::unique_ptr<ast> condition;
  std::vector<std::unique_ptr<ast>> body;

  WhileNode(std::unique_ptr<ast> cond, std::vector<std::unique_ptr<ast>> b)
      : condition(std::move(cond)), body(std::move(b)) {}

  std::string repr() override {
    std::string s = "WhileNode(cond=" + condition->repr() + ", body=[";
    for (auto &n : body)
      s += n->repr() + ",";
    s += "])";
    return s;
  }
};

class ForNode : public ast {
public:
  std::unique_ptr<ast> init;
  std::unique_ptr<ast> condition;
  std::unique_ptr<ast> increment;
  std::vector<std::unique_ptr<ast>> body;

  ForNode(std::unique_ptr<ast> i, std::unique_ptr<ast> cond,
          std::unique_ptr<ast> inc, std::vector<std::unique_ptr<ast>> b)
      : init(std::move(i)), condition(std::move(cond)),
        increment(std::move(inc)), body(std::move(b)) {}

  std::string repr() override {
    std::string s = "ForNode(init=" + (init ? init->repr() : "null") +
                    ", cond=" + (condition ? condition->repr() : "null") +
                    ", inc=" + (increment ? increment->repr() : "null") +
                    ", body=[";
    for (auto &n : body)
      s += n->repr() + ",";
    s += "])";
    return s;
  }
};

class FunctionNode : public ast {
public:
  std::string name;
  std::vector<std::string> parameters;
  std::vector<std::unique_ptr<ast>> body;

  FunctionNode(const std::string &n, std::vector<std::string> params,
               std::vector<std::unique_ptr<ast>> b)
      : name(n), parameters(std::move(params)), body(std::move(b)) {}

  std::string repr() override {
    std::string s = "FunctionNode(name=" + name + ", params=[";
    for (auto &p : parameters)
      s += p + ",";
    s += "], body=[";
    for (auto &n : body)
      s += n->repr() + ",";
    s += "])";
    return s;
  }
};


class PrintNode : ast {
public:
  std::unique_ptr<ast> expr;

  PrintNode(std::unique_ptr<ast> e) : expr(std::move(e)) {}

  llvm::Value *codegen() override {
    llvm::Value *val = expr->codegen();
    if (!val)
      return nullptr;

    llvm::Function *printfFunc = TheModule->getFunction("printf");
    if (!printfFunc) {
      // printf prototype: int printf(const char*, ...)
      std::vector<llvm::Type *> printfArgs;
      printfArgs.push_back(Builder->getInt8Ty());
      llvm::FunctionType *printfType =
          llvm::FunctionType::get(Builder->getInt32Ty(), printfArgs, true);
      printfFunc = llvm::Function::Create(
          printfType, llvm::Function::ExternalLinkage, "printf", TheModule.get());
    }

    llvm::Value *formatStr = nullptr;

    if (val->getType()->isIntegerTy(32)) {
      formatStr = Builder->CreateGlobalStringPtr("%d\n");
    } else if (val->getType()->isFloatTy()) {
      formatStr = Builder->CreateGlobalStringPtr("%f\n");
    } else if (val->getType()->isPointerTy()) {
      // assume pointer is a string (global string constant)
      formatStr = Builder->CreateGlobalStringPtr("%s\n");
    } else {
      fprintf(stderr, "Unsupported type for print\n");
      return nullptr;
    }

    llvm::Value *formatStrPtr = Builder->CreatePointerCast(formatStr, llvm::Type::getInt8Ty(*TheContext)->getPointerTo());
    return Builder->CreateCall(printfFunc, {formatStrPtr, val}, "printfCall");
  }

  std::string repr() override {
    return "PrintNode(" + (expr ? expr->repr() : "null") + ")";
  }
};






