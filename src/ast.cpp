// AST.cpp - merged single-file AST + codegen
// Compatible with your llvm-18 include paths (adjust if needed).

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include <llvm-18/llvm/IR/LLVMContext.h>
#include <llvm-18/llvm/IR/IRBuilder.h>
#include <llvm-18/llvm/IR/Module.h>
#include <llvm-18/llvm/IR/Verifier.h>
#include <llvm-18/llvm/IR/Function.h>
#include <llvm-18/llvm/IR/Type.h>
#include <llvm-18/llvm/IR/Constants.h>

using namespace std;
using namespace llvm;

// --- Globals (single module/context/builder) ---
static LLVMContext TheContext;
static unique_ptr<Module> TheModule = make_unique<Module>("my_module", TheContext);
static IRBuilder<> Builder(TheContext);
static unordered_map<string, Value *> NamedValue; // variable name -> alloca (or value)

// --- AST base ---
struct ast {
  virtual string repr() = 0;
  virtual Value *codegen(LLVMContext &context, IRBuilder<> &builder) = 0;
  virtual ~ast() = default;
};

// --- Helpers ---
static AllocaInst *CreateEntryBlockAlloca(Function *TheFunction, Type *Ty, const string &VarName) {
  IRBuilder<> TmpB(&TheFunction->getEntryBlock(), TheFunction->getEntryBlock().begin());
  return TmpB.CreateAlloca(Ty, nullptr, VarName);
}

static Function *getPrintfDeclaration(Module *M) {
  if (auto *F = M->getFunction("printf")) return F;
  // int printf(const char*, ...)
  FunctionType *printfType = FunctionType::get(IntegerType::getInt32(TheContext),
                                               PointerType::getUnqual(Type::getInt8Ty(TheContext)),
                                               true);
  return Function::Create(printfType, Function::ExternalLinkage, "printf", M);
}

// --- Nodes ---

// NumberNode - represents an integer literal (extendable)
class NumberNode : public ast {
public:
  int number;
  explicit NumberNode(int n) : number(n) {}
  Value *codegen(LLVMContext &context, IRBuilder<> &builder) override {
    return ConstantInt::get(Type::getInt32Ty(context), number, true);
  }
  string repr() override { return "NumberNode(" + to_string(number) + ")"; }
};

// StringNode
class StringNode : public ast {
public:
  string value;
  string Type;
  explicit StringNode(const string &v) : value(v) {}
  Value *codegen(LLVMContext &context, IRBuilder<> &builder) override {
    return builder.CreateGlobalStringPtr(value, "str");
  }
  string repr() override { return "StringNode(" + value + ")"; }
};

// VariableNode - referencing a variable (load its value)
class VariableNode : public ast {
public:
  string name;
  string type;
  VariableNode(const string &n, const string &t) : name(n), type(t) {}
  Value *codegen(LLVMContext &context, IRBuilder<> &builder) override {
    auto it = NamedValue.find(name);
    if (it == NamedValue.end()) {
      cerr << "Unknown variable: " << name << "\n";
      return nullptr;
    }
    Value *V = it->second;
    // If we have a pointer (allocated), load it
    if (V->getType()->isPointerTy()) {
      Type *elemTy = V->getType()->getPointerElementType();
      return builder.CreateLoad(elemTy, V, name + ".load");
    }
    // otherwise return value directly
    return V;
  }
  string repr() override { return "VariableNode(" + name + ")"; }
};

// VariableDeclareNode - allocate a variable in current function's entry block
class VariableDeclareNode : public ast {
public:
  string name;
  string type;
  VariableDeclareNode(const string &n, const string &t) : name(n), type(t) {}
  Value *codegen(LLVMContext &context, IRBuilder<> &builder) override {
    Function *TheFunction = builder.GetInsertBlock()->getParent();
    Type *llvmTy = nullptr;
    if (type == "INT") llvmTy = Type::getInt32Ty(context);
    else if (type == "FLOAT") llvmTy = Type::getFloatTy(context);
    else if (type == "STRING") llvmTy = Type::getInt8PtrTy(context);
    else {
      cerr << "Unknown type in declaration: " << type << "\n";
      return nullptr;
    }
    AllocaInst *alloca = CreateEntryBlockAlloca(TheFunction, llvmTy, name);
    NamedValue[name] = alloca;
    return alloca;
  }
  string repr() override { return "VariableDeclareNode(" + name + ", " + type + ")"; }
};

// AssignmentNode - store evaluated value into variable pointer
class AssignmentNode : public ast {
public:
  unique_ptr<VariableNode> name;
  vector<unique_ptr<ast>> value; // we assume single-expression value[0]
  string type;
  AssignmentNode(unique_ptr<VariableNode> n, vector<unique_ptr<ast>> v, const string &t = "")
      : name(move(n)), value(move(v)), type(t) {}
  Value *codegen(LLVMContext &context, IRBuilder<> &builder) override {
    if (value.empty()) { cerr << "Assignment missing RHS\n"; return nullptr; }
    Value *rhs = value[0]->codegen(context, builder);
    if (!rhs) return nullptr;
    auto it = NamedValue.find(name->name);
    if (it == NamedValue.end()) {
      cerr << "Attempt to assign to unknown variable: " << name->name << "\n";
      return nullptr;
    }
    Value *ptr = it->second;
    if (!ptr->getType()->isPointerTy()) {
      cerr << "Target is not a pointer for assignment\n";
      return nullptr;
    }
    Type *elemTy = ptr->getType()->getPointerElementType();
    // if types mismatch and rhs is integer constant convertible to elemTy int, attempt cast
    if (rhs->getType() != elemTy) {
      if (rhs->getType()->isIntegerTy() && elemTy->isIntegerTy()) {
        rhs = builder.CreateIntCast(rhs, elemTy, true, "casttmp");
      } else if (rhs->getType()->isIntegerTy() && elemTy->isFloatTy()) {
        rhs = builder.CreateSIToFP(rhs, elemTy, "sitofp");
      } else if (rhs->getType()->isFloatTy() && elemTy->isIntegerTy()) {
        rhs = builder.CreateFPToSI(rhs, elemTy, "fptosi");
      } // more casts could be added
    }
    return builder.CreateStore(rhs, ptr);
  }
  string repr() override { return "AssignmentNode"; }
};

// BinaryOperationNode - supports int/float basic ops
class BinaryOperationNode : public ast {
public:
  unique_ptr<ast> left;
  unique_ptr<ast> right;
  string op;
  BinaryOperationNode(unique_ptr<ast> l, unique_ptr<ast> r, const string &o)
      : left(move(l)), right(move(r)), op(o) {}
  Value *codegen(LLVMContext &context, IRBuilder<> &builder) override {
    Value *L = left->codegen(context, builder);
    Value *R = right->codegen(context, builder);
    if (!L || !R) return nullptr;
    // promote ints to floats if necessary
    if (L->getType()->isFloatTy() || R->getType()->isFloatTy()) {
      if (!L->getType()->isFloatTy())
        L = builder.CreateSIToFP(L, Type::getFloatTy(context), "promL");
      if (!R->getType()->isFloatTy())
        R = builder.CreateSIToFP(R, Type::getFloatTy(context), "promR");
      if (op == "+") return builder.CreateFAdd(L, R, "addtmp");
      if (op == "-") return builder.CreateFSub(L, R, "subtmp");
      if (op == "*") return builder.CreateFMul(L, R, "multmp");
      if (op == "/") return builder.CreateFDiv(L, R, "divtmp");
    } else {
      // integer ops
      if (op == "+") return builder.CreateAdd(L, R, "addtmp");
      if (op == "-") return builder.CreateSub(L, R, "subtmp");
      if (op == "*") return builder.CreateMul(L, R, "multmp");
      if (op == "/") return builder.CreateSDiv(L, R, "divtmp");
    }
    cerr << "Unsupported binary operator: " << op << "\n";
    return nullptr;
  }
  string repr() override { return "BinaryOp(" + op + ")"; }
};

// IdentifierNode - similar to VariableNode but by id string (keeps parser compatibility)
class IdentifierNode : public ast {
public:
  string id;
  explicit IdentifierNode(const string &n) : id(n) {}
  Value *codegen(LLVMContext &context, IRBuilder<> &builder) override {
    auto it = NamedValue.find(id);
    if (it == NamedValue.end()) {
      cerr << "Unknown identifier: " << id << "\n";
      return nullptr;
    }
    Value *V = it->second;
    if (V->getType()->isPointerTy()) {
      return builder.CreateLoad(V->getType()->getPointerElementType(), V, id + ".load");
    }
    return V;
  }
  string repr() override { return "Identifier(" + id + ")"; }
};

// PrintNode - simple printf wrapper supporting string and int
class PrintNode : public ast {
public:
  unique_ptr<ast> expr;
  explicit PrintNode(unique_ptr<ast> e) : expr(move(e)) {}
  Value *codegen(LLVMContext &context, IRBuilder<> &builder) override {
    Value *val = expr->codegen(context, builder);
    if (!val) return nullptr;
    Function *printfF = getPrintfDeclaration(TheModule.get());
    // if string
    if (val->getType()->isPointerTy() && val->getType()->getPointerElementType()->isIntegerTy(8)) {
      // printf("%s\n", str)
      Value *fmt = builder.CreateGlobalStringPtr("%s\n", "fmtstr");
      return builder.CreateCall(printfF, {fmt, val});
    }
    // if int
    if (val->getType()->isIntegerTy()) {
      Value *fmt = builder.CreateGlobalStringPtr("%d\n", "fmtint");
      Value *toPass = val;
      if (val->getType()->getIntegerBitWidth() != 32)
        toPass = builder.CreateIntCast(val, Type::getInt32Ty(context), true);
      return builder.CreateCall(printfF, {fmt, toPass});
    }
    // if float
    if (val->getType()->isFloatTy()) {
      // printf requires double promotion
      Value *fmt = builder.CreateGlobalStringPtr("%f\n", "fmtflt");
      Value *asDouble = builder.CreateFPExt(val, Type::getDoubleTy(context));
      return builder.CreateCall(printfF, {fmt, asDouble});
    }
    cerr << "Unsupported print type\n";
    return nullptr;
  }
  string repr() override { return "PrintNode"; }
};

// Return/Break/Continue/Control nodes - stubs for completeness
class ReturnNode : public ast {
public:
  unique_ptr<ast> expr;
  explicit ReturnNode(unique_ptr<ast> e = nullptr) : expr(move(e)) {}
  Value *codegen(LLVMContext &context, IRBuilder<> &builder) override {
    if (expr) {
      Value *v = expr->codegen(context, builder);
      return builder.CreateRet(v);
    }
    return builder.CreateRetVoid();
  }
  string repr() override { return "ReturnNode"; }
};
class BreakNode : public ast {
public:
  Value *codegen(LLVMContext &context, IRBuilder<> &builder) override { return nullptr; }
  string repr() override { return "BreakNode"; }
};
class ContinueNode : public ast {
public:
  Value *codegen(LLVMContext &context, IRBuilder<> &builder) override { return nullptr; }
  string repr() override { return "ContinueNode"; }
};

// Other nodes (If/While/For/Comparison/Function) are omitted for brevity but can be added similarly.

// --- Example usage helper (not required) ---
// You can build a simple function, declare variables and emit IR using the nodes above.
// Example is intentionally omitted to keep file focused on merged AST + codegen.


