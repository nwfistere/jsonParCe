#pragma once

#include "expressions.hpp"
#include "json_node.hpp"
#include "json_parce.h"
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace json_path {

struct json_node;

class filter_expression {
private:
  std::string m_expression;
  node_provider_t m_provider;

public:
  filter_expression(json_node_t root, json_node_t current,
                    const std::string &expression)
      : m_expression(expression),
        m_provider(std::make_shared<node_provider>(root, current)) {}
  json_node_t parse();
};

struct expression_parser {
  node_provider_t m_provider;
  std::vector<expression_t> expressions;
  expression_parser(node_provider_t provider) : m_provider(provider) {}
  ~expression_parser() = default;

  void cleanup_expressions(bool final);

  std::string get_selector(std::string str);

  std::vector<expression_t> parse(std::string str);

  void add_expression(expression_t expr) { expressions.push_back(expr); }
};

} // namespace json_path