#pragma once
#include <stdexcept>
#include <variant>
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
  MULTI,
  WILDCARD
};

struct expression {
  OPERATION operation;
  expression(OPERATION operation) : operation(operation) {}
  virtual ~expression() = default;
  virtual bool eval() = 0;
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

struct value_expression : public expression {
  value_expression(OPERATION operation) : expression(operation) {}
  bool eval() override = 0;

  virtual json_node_t get_value() = 0;

  static bool is_value(expression_t expr) {
    return (dynamic_cast<value_expression*>(expr.get()) != nullptr);
  }
};

using value_expression_t = std::shared_ptr<value_expression>;

struct binary_expression : public expression {
  expression_t left;
  expression_t right;

  binary_expression(expression_t l, OPERATION operation, expression_t r) :
  expression(operation),
  left(l),
  right(r) {}

  void get_child_expression_values(value_expression_t& l_value, bool* l_bool, value_expression_t& r_value, bool* r_bool) {
    if (value_expression::is_value(left)) {
      l_value = std::dynamic_pointer_cast<value_expression>(left);
    } else {
      *l_bool = left->eval();
    }

    if (value_expression::is_value(right)) {
      r_value = std::dynamic_pointer_cast<value_expression>(right);
    } else {
      *r_bool = right->eval();
    }
  }

  virtual bool eval() override = 0;

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

  virtual bool eval() override {
    bool l_bool;
    bool r_bool;
    value_expression_t l_value;
    value_expression_t r_value;

    get_child_expression_values(l_value, &l_bool, r_value, &r_bool);
    
    if (l_value != nullptr) {
      if (r_value != nullptr) {
        return *(l_value->get_value()) || *(r_value->get_value());
      }
      return *(l_value->get_value()) || r_bool;
    }
    if (r_value != nullptr) {
      return l_bool || *(r_value->get_value());
    }
    return l_bool || r_bool;
  }
};

struct and_expression : public binary_expression {
  and_expression(expression_t l, expression_t r) : binary_expression(l, OPERATION::AND, r) {}

  virtual bool eval() override {
    bool l_bool;
    bool r_bool;
    value_expression_t l_value;
    value_expression_t r_value;

    get_child_expression_values(l_value, &l_bool, r_value, &r_bool);
    
    if (l_value != nullptr) {
      if (r_value != nullptr) {
        return *(l_value->get_value()) && *(r_value->get_value());
      }
      return *(l_value->get_value()) && r_bool;
    }
    if (r_value != nullptr) {
      return l_bool && *(r_value->get_value());
    }
    return l_bool && r_bool;
  }
};

struct equivalent_expression : public binary_expression {
  equivalent_expression(expression_t l, expression_t r) : binary_expression(l, OPERATION::EQUAL, r) {}

  virtual bool eval() override {
    if (value_expression::is_value(left) && value_expression::is_value(right)) {
      value_expression* l_result = dynamic_cast<value_expression*>(left.get());
      value_expression* r_result = dynamic_cast<value_expression*>(right.get());
      return (*l_result->get_value()) == (*r_result->get_value());
    } else if (value_expression::is_value(left)) {
      value_expression* l_result = dynamic_cast<value_expression*>(left.get());
      bool r_result = right->eval();
      return (*l_result->get_value()) == r_result;
    } else if (value_expression::is_value(right)) {
      bool l_result = left->eval();
      value_expression* r_result = dynamic_cast<value_expression*>(right.get());
      return l_result == (*r_result->get_value());
    }
    bool l_result = left->eval();
    bool r_result = right->eval();

    return (l_result == r_result);
  }
};

struct not_equivalent_expression : public binary_expression {
  not_equivalent_expression(expression_t l, expression_t r) : binary_expression(l, OPERATION::NOT_EQUAL, r) {}


  virtual bool eval() override {
    bool l_bool;
    bool r_bool;
    value_expression_t l_value;
    value_expression_t r_value;

    get_child_expression_values(l_value, &l_bool, r_value, &r_bool);
    
    if (l_value != nullptr) {
      if (r_value != nullptr) {
        return *(l_value->get_value()) != *(r_value->get_value());
      }
      return *(l_value->get_value()) != r_bool;
    }
    if (r_value != nullptr) {
      return l_bool != *(r_value->get_value());
    }
    return l_bool != r_bool;
  }
};

struct le_expression : public binary_expression {
  le_expression(expression_t l, expression_t r) : binary_expression(l, OPERATION::LE, r) {}

  virtual bool eval() override {
    value_expression_t l_value = std::dynamic_pointer_cast<value_expression>(left);
    value_expression_t r_value =std::dynamic_pointer_cast<value_expression>(right);
    
    return *l_value->get_value() <= *r_value->get_value();
  }
};

struct lt_expression : public binary_expression {
  lt_expression(expression_t l, expression_t r) : binary_expression(l, OPERATION::LT, r) {}

  virtual bool eval() override {
    value_expression_t l_value = std::dynamic_pointer_cast<value_expression>(left);
    value_expression_t r_value =std::dynamic_pointer_cast<value_expression>(right);
    
    return *l_value->get_value() < *r_value->get_value();
  }
};

struct ge_expression : public binary_expression {
  ge_expression(expression_t l, expression_t r) : binary_expression(l, OPERATION::GE, r) {}

  virtual bool eval() override {
    value_expression_t l_value = std::dynamic_pointer_cast<value_expression>(left);
    value_expression_t r_value =std::dynamic_pointer_cast<value_expression>(right);
    
    return *l_value->get_value() >= *r_value->get_value();
  }
};

struct gt_expression : public binary_expression {
  gt_expression(expression_t l, expression_t r) : binary_expression(l, OPERATION::GT, r) {}

  virtual bool eval() override {
    value_expression_t l_value = std::dynamic_pointer_cast<value_expression>(left);
    value_expression_t r_value =std::dynamic_pointer_cast<value_expression>(right);
    
    return *l_value->get_value() > *r_value->get_value();
  }
};

struct not_expression : public unary_expression {
  not_expression(expression_t expr) : unary_expression(OPERATION::NOT, expr) {}

  virtual bool eval() override {
    return !expr->eval();
  }
};

struct grouped_expression : public unary_expression {
  grouped_expression(expression_t expr) : unary_expression(OPERATION::PARENTHESES, expr) {}

  virtual bool eval() override {
    return expr->eval();
  }
};

struct literal_value_expression : public value_expression {
  json_node_t value;
  literal_value_expression(json_node_t node) :
    value_expression(LITERAL),
    value(node) {}

  bool eval() override {
    return (bool) (*get_value());
  }

  json_node_t get_value() override {
    return value;
  }
};

struct selector_child_expression : public literal_value_expression {
  selector_child_expression(json_node_t node) :
    literal_value_expression(node) {}
};

struct selector_expression : public value_expression {
  value_expression_t left;
  std::shared_ptr<selector_child_expression> right;
  selector_expression(value_expression_t l, std::shared_ptr<selector_child_expression> r) :
    value_expression(OPERATION::SELECTOR), left(l), right(r) {}

  bool eval() override {
    return (bool) (*get_value());
  }

  json_node_t get_value() override {
    std::string selector;
    json_node_t l_result = left->get_value();
    json_node_t r_result = right->get_value();
    if (r_result->type == STRING) {
      // regular selector;
      selector = right->get_value()->as<std::string>();
    } else {
      // wildcard // TODO: wildcard expression could probably go away with just a check here to do it's logic.
      return r_result;
    }
    
    try {
      value_t val = l_result->as<std::map<std::string, json_node>>().at(selector).value;
      return std::make_shared<json_node>(val);
    } catch (std::out_of_range) {} catch (std::bad_variant_access) {}
  

    return std::make_shared<json_node>(std::monostate());
  }
};

struct provided_node_expression : public value_expression {
  node_provider_t m_provider;
  provided_node_expression(node_provider_t provider) :
    value_expression(LITERAL), m_provider(provider) {}

    bool eval() override {
      throw std::invalid_argument("invalid type");
    }
};

struct current_node_expression : public provided_node_expression {
  current_node_expression(node_provider_t node) :
    provided_node_expression(node) {}

  json_node_t get_value() override {
    return m_provider->get_current_node();
  }
};

struct root_node_expression : public provided_node_expression {
  root_node_expression(node_provider_t node) :
    provided_node_expression(node) {}

  json_node_t get_value() override {
    return m_provider->get_root_node();
  }
};

struct match_function_expression : public binary_expression {
  match_function_expression(expression_t l, expression_t r) :
    binary_expression(l, OPERATION::FUNCTION, r) {}

  bool eval() override {
    return true; // TODO: implement
  }
};

struct search_function_expression : public binary_expression {
  search_function_expression(expression_t l, expression_t r) :
    binary_expression(l, OPERATION::FUNCTION, r) {}

  bool eval() override {
    return true; // TODO: implement
  }
};

struct sub_filter_expression : public value_expression {
  node_provider_t m_provider;
  value_expression_t outer_expr;
  std::shared_ptr<literal_value_expression> inner_expr;

  // TODO: inherit unary_expression and provided_expressions, they will need to be virtual.
  sub_filter_expression(
    value_expression_t outer_expr,
    std::shared_ptr<literal_value_expression> inner_expr,
    node_provider_t provider) :
    value_expression(OPERATION::FILTER),
    m_provider(provider),
    outer_expr(outer_expr),
    inner_expr(inner_expr) {}

  bool eval() override {
    // throw std::invalid_argument("Invalid type to call eval on.");
    return (bool) (*get_value());
  }

  json_node_t get_value() override;
};

struct multi_filter_wrapper_expression : public expression {
  expression_t expr;
  multi_filter_wrapper_expression(expression_t expr)
    : expression(MULTI),
    expr(expr) {}

  bool eval() {
    return expr->eval();
  }
};

struct wildcard_expression : public selector_child_expression {
  node_provider_t m_provider;
  wildcard_expression(node_provider_t provider) :
    selector_child_expression(nullptr),
    m_provider(provider) {}

  // bool eval() override {
  //   return (bool) (*get_value());
  // }

  json_node_t get_value() override;
};

} // namespace json_path

#endif // EXPRESSIONS_H