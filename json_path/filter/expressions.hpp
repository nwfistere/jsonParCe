#pragma once
#ifndef EXPRESSIONS_H
#define EXPRESSIONS_H

#include "json_node.hpp"
#include <memory>

namespace json_path {

enum OPERATION {
  OP_NONE = 0,
  OR,
  AND,
  // RELATION,
  EQUAL,
  NOT_EQUAL,
  LT,
  LE,
  GT,
  GE,
  NOT,
  PARENTHESES,
  FUNCTION,
  SELECTOR
};

struct expression {
  virtual ~expression() = default;
  virtual json_node eval(json_node node) = 0;
  virtual json_node eval() = 0;
};

using expression_t = std::shared_ptr<expression>;

class node_provider {
  json_node_t current_node;
  json_node_t root_node;
public:
  node_provider() = default;
  node_provider(json_node_t root, json_node_t current): current_node(nullptr), root_node(root) {}
  ~node_provider() = default;

  json_node_t get_current_node() {
    return current_node;
  }

  json_node_t set_current_node(json_node_t node) {
    current_node = node;
  }

  json_node_t get_root_node() {
    return root_node;
  }

  json_node_t set_root_node(json_node_t node) {
    root_node = node;
  }
};

using node_provider_t = std::shared_ptr<node_provider>;

struct binary_expression : public expression {
  expression_t left;
  OPERATION operation;
  expression_t right;

  binary_expression(expression_t l, OPERATION operation, expression_t r) :
  left(l),
  operation(operation),
  right(r) {}

  virtual json_node eval(json_node node) override = 0;
  json_node eval() override {
    throw std::runtime_error("invalid operation " + std::to_string(operation));
  }

  bool has_left() {
    return left != nullptr;
  }

  bool has_right() {
    return right != nullptr;
  }

  void set_right(expression_t new_right) {
    right = new_right;
  }
};

struct unary_expression : public expression {
  expression_t expr;
  OPERATION operation;

  unary_expression(OPERATION operation, expression_t expr) :
  expr(expr),
  operation(operation) {}

  virtual json_node eval(json_node node) override = 0;

  json_node eval() override {
    throw std::runtime_error("invalid operation " + std::to_string(operation));
  }
};

struct or_expression : public binary_expression {
  or_expression(expression_t l, expression_t r) : binary_expression(l, OPERATION::OR, r) {}

  virtual json_node eval(json_node node) override {
    json_node l_result = left->eval(node);
    json_node r_result = right->eval(node);

    std::vector<json_node> result = l_result.as<std::vector<json_node>>();
    for (const auto& i : r_result.as<std::vector<json_node>>()) {
      result.push_back(i);
    }

    return json_node(result);
  }
};

struct and_expression : public binary_expression {
  and_expression(expression_t l, expression_t r) : binary_expression(l, OPERATION::AND, r) {}

  virtual json_node eval(json_node node) override {
    json_node l_result = left->eval(node);
    json_node r_result = right->eval(node);

    if (l_result && r_result) {
      return l_result;
    }
    return json_node(std::monostate());
  }
};

struct equivalent_expression : public binary_expression {
  equivalent_expression(expression_t l, expression_t r) : binary_expression(l, OPERATION::EQUAL, r) {}

  virtual json_node eval(json_node node) override {
    json_node l_result = left->eval(node);
    json_node r_result = right->eval(node);

    if (l_result == r_result) {
      return l_result;
    }
    return json_node(std::vector<json_node>());
  }
};

struct not_equivalent_expression : public binary_expression {
  not_equivalent_expression(expression_t l, expression_t r) : binary_expression(l, OPERATION::NOT_EQUAL, r) {}

  virtual json_node eval(json_node node) override {
    json_node l_result = left->eval(node);
    json_node r_result = right->eval(node);

    if (l_result != r_result) {
      return l_result;
    }
    return json_node(std::vector<json_node>());
  }
};

struct le_expression : public binary_expression {
  le_expression(expression_t l, expression_t r) : binary_expression(l, OPERATION::LE, r) {}

  virtual json_node eval(json_node node) override {
    json_node l_result = left->eval(node);
    json_node r_result = right->eval(node);

    if (l_result <= r_result) {
      return l_result;
    }
    return json_node(std::vector<json_node>());
  }
};

struct lt_expression : public binary_expression {
  lt_expression(expression_t l, expression_t r) : binary_expression(l, OPERATION::LT, r) {}

  virtual json_node eval(json_node node) override {
    json_node l_result = left->eval(node);
    json_node r_result = right->eval(node);

    if (l_result < r_result) {
      return l_result;
    }
    return json_node(std::vector<json_node>());
  }
};

struct ge_expression : public binary_expression {
  ge_expression(expression_t l, expression_t r) : binary_expression(l, OPERATION::GE, r) {}

  virtual json_node eval(json_node node) override {
    json_node l_result = left->eval(node);
    json_node r_result = right->eval(node);

    if (l_result >= r_result) {
      return l_result;
    }
    return json_node(std::vector<json_node>());
  }
};

struct gt_expression : public binary_expression {
  gt_expression(expression_t l, expression_t r) : binary_expression(l, OPERATION::GT, r) {}

  virtual json_node eval(json_node node) override {
    json_node l_result = left->eval(node);
    json_node r_result = right->eval(node);

    if (l_result > r_result) {
      return l_result;
    }
    return json_node(std::vector<json_node>());
  }
};

struct not_expression : public unary_expression {
  not_expression(expression_t expr) : unary_expression(OPERATION::NOT, expr) {}

  virtual json_node eval(json_node node) override {
    json_node result = expr->eval(node);
    // TODO: on construction, we need to invert the child expressions.
    return result;
  }
};

struct grouped_expression : public unary_expression {
  grouped_expression(expression_t expr) : unary_expression(OPERATION::PARENTHESES, expr) {}

  virtual json_node eval(json_node node) override {
    json_node result = expr->eval(node);

    return result;
  }
};

struct function_expression : public unary_expression { // TODO: Will likely need to break this one up...
  function_expression(expression_t expr) : unary_expression(OPERATION::FUNCTION, expr) {}

  virtual json_node eval(json_node node) override {
    json_node result = expr->eval(node);

    return result;
  }
};

struct literal_value_expression : public expression {
  json_node_t node;

  literal_value_expression(json_node* node) : node(node) {}

  virtual json_node eval() override {
    return *node;
  }

  json_node eval(json_node) override {
    throw std::invalid_argument("Invalid expression to call eval(json_node) on.");
  }
};

struct selector_child_expression : public literal_value_expression {
  selector_child_expression(json_node* node) :
    literal_value_expression(node) {}

  using literal_value_expression::eval;
};

struct selector_expression : public binary_expression {
  selector_expression(expression_t l, expression_t r) : binary_expression(l, OPERATION::SELECTOR, r) {}

  virtual json_node eval(json_node node) override {
    json_node l_result = left->eval();

    std::map<std::string, json_node> object = l_result.as<std::map<std::string, json_node>>();
    selector_child_expression* child_expresion = nullptr;

    try {
      dynamic_cast<selector_child_expression*>(right.get());
    } catch (const std::bad_cast& e) {
      std::cout << "bad_cast to selector_child_expression\n";
      std::cout << e.what() << "\n";
      throw e;
    }

    std::string child = child_expresion->node->as<std::string>();

    return object.at(child);
  }
};

struct provided_node_expression : public expression {
  node_provider_t m_provider;
  provided_node_expression(node_provider_t provider) : m_provider(provider) {}
};

struct current_node_expression : public provided_node_expression {
  current_node_expression(node_provider_t node) :
    provided_node_expression(node) {}

  json_node eval() override {
    return *m_provider->get_current_node().get();
  }

  json_node eval(json_node) override {
    throw std::invalid_argument("Invalid expression to call eval(json_node) on.");
  }
};

struct root_node_expression : public provided_node_expression {
  root_node_expression(node_provider_t node) :
    provided_node_expression(node) {}

  json_node eval() override {
    return *m_provider->get_root_node().get();
  }

  json_node eval(json_node) override {
    throw std::invalid_argument("Invalid expression to call eval(json_node) on.");
  }
};

} // namespace json_path

#endif // EXPRESSIONS_H