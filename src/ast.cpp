#include <memory>
#include <string>
#include <utility>
#include <vector>

struct ast {
  virtual std::string repr() = 0;
  virtual ~ast() {}
};

// ------------------ Expression / Value Nodes ------------------

class NumberNode : public ast {
public:
  int number;
  NumberNode(int n) : number(n) {}
  std::string repr() override {
    return "NumberNode(" + std::to_string(number) + ")";
  }
};

class VariableNode : public ast {
public:
  std::string name;
  VariableNode(const std::string &n) : name(n) {}
  std::string repr() override { return "VariableNode(" + name + ")"; }
};

class VariableDeclareNode : public ast {
public:
  std::string name;
  std::string type;
  VariableDeclareNode(const std::string &n, const std::string &tp)
      : name(n), type(tp) {}
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
