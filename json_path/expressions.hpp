#pragma once
#include <stdexcept>
#ifndef EXPRESSIONS_H
#define EXPRESSIONS_H

#include "json_node.hpp"
#include <memory>

namespace json_path {

enum OPERATION {
  OP_NONE = 0,
  OR,
  AND,
  EQUAL,
  NOT_EQUAL,
  LT,
  LE,
  GT,
  GE,
  NOT,
  PARENTHESES,
  FUNCTION,
  SELECTOR,
  LITERAL,
  FILTER,
  MULTI
};

struct expression {
  OPERATION operation;
  expression(OPERATION operation) : operation(operation) {}
  virtual ~expression() = default;
  virtual json_node eval() = 0;
};

using expression_t = std::shared_ptr<expression>;

class node_provider {
  json_node_t current_node;
  json_node_t root_node;
public:
  node_provider() = default;
  node_provider(json_node_t root, json_node_t current): current_node(current), root_node(root) {}
  ~node_provider() = default;

  json_node_t get_current_node() {
    return current_node;
  }

  void set_current_node(json_node_t node) {
    current_node = node;
  }

  json_node_t get_root_node() {
    return root_node;
  }

  void set_root_node(json_node_t node) {
    root_node = node;
  }
};

using node_provider_t = std::shared_ptr<node_provider>;

struct binary_expression : public expression {
  expression_t left;
  expression_t right;

  binary_expression(expression_t l, OPERATION operation, expression_t r) :
  expression(operation),
  left(l),
  right(r) {}

  virtual json_node eval() override = 0;

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

  unary_expression(OPERATION operation, expression_t expr) :
    expression(operation),
    expr(expr) {}

};

struct or_expression : public binary_expression {
  or_expression(expression_t l, expression_t r) : binary_expression(l, OPERATION::OR, r) {}

  virtual json_node eval() override {
    json_node l_result = left->eval();
    json_node r_result = right->eval();

    std::vector<json_node> result;
    if (l_result.type == ARRAY) {
      result = l_result.as<std::vector<json_node>>();
    } else if (l_result.type != NONE) {
      result.push_back(l_result);
    }

    if (r_result.type == ARRAY) {
      for (const auto& i : r_result.as<std::vector<json_node>>()) {
        result.push_back(i);
      }
    } else if (r_result.type != NONE) {
      result.push_back(r_result);
    }

    if (result.empty()) {
      return json_node(std::monostate());
    }

    return json_node(result);
  }
};

struct and_expression : public binary_expression {
  and_expression(expression_t l, expression_t r) : binary_expression(l, OPERATION::AND, r) {}

  virtual json_node eval() override {
    json_node l_result = left->eval();
    json_node r_result = right->eval();

    if (l_result && r_result) {
      return l_result;
    }
    return json_node(std::monostate());
  }
};

struct equivalent_expression : public binary_expression {
  equivalent_expression(expression_t l, expression_t r) : binary_expression(l, OPERATION::EQUAL, r) {}

  virtual json_node eval() override {
    json_node l_result = left->eval();
    json_node r_result = right->eval();

    if (l_result == r_result) {
      return l_result;
    }
    return json_node(std::monostate());
  }
};

struct not_equivalent_expression : public binary_expression {
  not_equivalent_expression(expression_t l, expression_t r) : binary_expression(l, OPERATION::NOT_EQUAL, r) {}

  virtual json_node eval() override {
    json_node l_result = left->eval();
    json_node r_result = right->eval();

    if (l_result != r_result) {
      return l_result;
    }
    return json_node(std::monostate());
  }
};

struct le_expression : public binary_expression {
  le_expression(expression_t l, expression_t r) : binary_expression(l, OPERATION::LE, r) {}

  virtual json_node eval() override {
    json_node l_result = left->eval();
    json_node r_result = right->eval();

    if (l_result <= r_result) {
      return l_result;
    }
    return json_node(std::monostate());
  }
};

struct lt_expression : public binary_expression {
  lt_expression(expression_t l, expression_t r) : binary_expression(l, OPERATION::LT, r) {}

  virtual json_node eval() override {
    json_node l_result = left->eval();
    json_node r_result = right->eval();

    if (l_result < r_result) {
      return l_result;
    }
    return json_node(std::monostate());
  }
};

struct ge_expression : public binary_expression {
  ge_expression(expression_t l, expression_t r) : binary_expression(l, OPERATION::GE, r) {}

  virtual json_node eval() override {
    json_node l_result = left->eval();
    json_node r_result = right->eval();

    if (l_result >= r_result) {
      return l_result;
    }
    return json_node(std::monostate());
  }
};

struct gt_expression : public binary_expression {
  gt_expression(expression_t l, expression_t r) : binary_expression(l, OPERATION::GT, r) {}

  virtual json_node eval() override {
    json_node l_result = left->eval();
    json_node r_result = right->eval();

    if (l_result > r_result) {
      return l_result;
    }
    return json_node(std::monostate());
  }
};

struct not_expression : public unary_expression {
  not_expression(expression_t expr) : unary_expression(OPERATION::NOT, expr) {}

  virtual json_node eval() override {
    json_node result = expr->eval();
    // TODO: on construction, we need to invert the child expressions.
    return result;
  }
};

struct grouped_expression : public unary_expression {
  grouped_expression(expression_t expr) : unary_expression(OPERATION::PARENTHESES, expr) {}

  virtual json_node eval() override {
    json_node result = expr->eval();

    return result;
  }
};

struct literal_value_expression : public expression {
  json_node_t node;

  literal_value_expression(json_node_t node) :
    expression(LITERAL), node(node) {}

  virtual json_node eval() override {
    return *node;
  }

};

struct selector_child_expression : public literal_value_expression {
  selector_child_expression(json_node_t node) :
    literal_value_expression(node) {}

  virtual json_node eval() override {
    return *node;
  }
};

struct selector_expression : public binary_expression {
  selector_expression(expression_t l, expression_t r) : binary_expression(l, OPERATION::SELECTOR, r) {}

  virtual json_node eval() override {
    json_node l_result = left->eval();

    if (l_result.type != OBJECT) {
      return json_node(std::monostate());
    }

    std::map<std::string, json_node> object = l_result.as<std::map<std::string, json_node>>();
    selector_child_expression* child_expresion = nullptr;

    child_expresion = dynamic_cast<selector_child_expression*>(right.get());
    if(child_expresion) {
      std::string child = child_expresion->node->as<std::string>();
      try {
        return object.at(child);
      } catch (std::out_of_range) {
        return json_node(std::monostate());
      }
    }
    throw std::invalid_argument("Expected selector_child_expression, got something else.");
  }
};

struct provided_node_expression : public expression {
  node_provider_t m_provider;
  provided_node_expression(node_provider_t provider) :
    expression(LITERAL), m_provider(provider) {}
};

struct current_node_expression : public provided_node_expression {
  current_node_expression(node_provider_t node) :
    provided_node_expression(node) {}

  json_node eval() override {
    return *m_provider->get_current_node().get();
  }
};

struct root_node_expression : public provided_node_expression {
  root_node_expression(node_provider_t node) :
    provided_node_expression(node) {}

  json_node eval() override {
    return *m_provider->get_root_node().get();
  }
};

struct match_function_expression : public binary_expression {
  match_function_expression(expression_t l, expression_t r) :
    binary_expression(l, OPERATION::FUNCTION, r) {}

  json_node eval() override {
    std::cout << "match.eval()\n";
    return left->eval();
  }
};

struct search_function_expression : public binary_expression {
  search_function_expression(expression_t l, expression_t r) :
    binary_expression(l, OPERATION::FUNCTION, r) {}

  json_node eval() override {
    std::cout << "search.eval()\n";
    return left->eval();
  }
};

struct sub_filter_expression : public binary_expression {
  node_provider_t m_provider;
  // TODO: inherit unary_expression and provided_expressions, they will need to be virtual.
  sub_filter_expression(
    expression_t outer_expr,
    std::shared_ptr<literal_value_expression> inner_expr,
    node_provider_t provider) :
    binary_expression(outer_expr, OPERATION::FILTER, inner_expr),
    m_provider(provider) {}

  json_node eval() override;
};

struct multi_filter_wrapper_expression : public unary_expression {
  multi_filter_wrapper_expression(expression_t expr)
    : unary_expression(MULTI, expr) {}

  json_node eval() {
    return expr->eval();
  }
};

} // namespace json_path

#endif // EXPRESSIONS_H