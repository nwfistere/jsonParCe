#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include "expressions.hpp"
#include "json_node.hpp"
#include "expressions.hpp"
#include "json_parce.h"

namespace json_path {

struct json_node;

class filter_expression {
private:
  std::string m_expression;
  node_provider_t m_provider;

public:
  filter_expression(json_node_t root, json_node_t current, const std::string& expression) :
    m_expression(expression),
    m_provider(std::make_shared<node_provider>(root, current)) {}
  json_node parse();
};

static std::string get_full_string(const std::string& statement);

static std::string get_bracket_substatement(const std::string& statement, const char lbracket, const char rbracket) {
  int level = 1; // assumes first left bracket is already cut off.
  size_t i;
  for (i = 0; i < statement.size(); i++) {
    if (statement[i] == rbracket) {
      level--;
      if (level == 0) {
        break;
      }
    } else if (statement[i] == lbracket) {
      level++;
    } else if (statement[i] == '\'' || statement[i] == '"') {
      i += get_full_string(statement.substr(i)).size() + 1; // +1 to get past the '\''
    }
  }
  if (level != 0) {
    throw std::invalid_argument("No end of bracket statement " + std::to_string(i));
  }
  return statement.substr(0, i);
}

static std::string get_full_parentheses_statement(const std::string& statement) {
  return get_bracket_substatement(statement, '(', ')');
}

static std::string get_full_sub_filter_statement(const std::string& statement) {
  return get_bracket_substatement(statement, '[', ']');
}

std::string get_full_string(const std::string& statement) {
  char delim = statement[0];
  size_t i;
  bool found = false;
  for (i = 1; i < statement.size(); i++) {
    if (statement[i] == delim) {
        found = true;
        break;
    } else if (statement[i] == '\\') {
      i++; // skip next character.
    }
  }
  if (!found) {
    throw std::invalid_argument("No end of string " + std::to_string(i));
  }
  return statement.substr(1, i - 1);
}

static void get_number(const std::string& str, json_parce_int_t& i, json_parce_real_t& j, JSON_PATH_TYPE& type, size_t& len) {
  size_t k = 0;
  while(str[k] && (((str[k] >= '0') && (str[k] <= '9')) || str[k] == '-' || str[k] == '+' || str[k] == 'e' || str[k] == 'E' || str[k] == '.')) { k++; }

  if (!json_parce_int(str.c_str(), k, &i)) {
    type = JSON_PATH_TYPE::INT_TYPE;
    j = 0;
    len = k;
  } else if (!json_parce_real(str.c_str(), k, &j)) {
    type = JSON_PATH_TYPE::REAL_TYPE;
    i = 0;
    len = k;

  } else {
    type = JSON_PATH_TYPE::NONE;
    i = 0;
    j = 0;
    len = (size_t) -1;
  }
}

static std::string get_function_parameter(const std::string& statement) {
  size_t i = 0;
  while(statement[i] != ',' && statement[i] != ')') {
    // TODO: probably more edge cases here, don't worry about them for now.
    // skip over string literals, they may have commas in them.
    if (i == '\'' || i == '"') {
      i += get_full_string(statement.substr(i)).size();
    } else if (i == '(') {
      i += get_full_parentheses_statement(statement.substr(i + 1)).size();
    }
    i++;
  }

  return statement.substr(0, i);
}

static std::string trim(std::string s) {
  s.erase(0, s.find_first_not_of(" \n\r\t"));
  s.erase(s.find_last_not_of(" \n\r\t")+1);
  return s;
}

struct expression_parser {
  node_provider_t m_provider;
  std::vector<expression_t> expressions;
  expression_parser(node_provider_t provider) : m_provider(provider) {}
  ~expression_parser() = default;

  void cleanup_expressions(bool final);

  std::string get_selector(std::string str);

  std::vector<expression_t> parse(std::string str);

  void add_expression(expression_t expr) {
    expressions.push_back(expr);
  }

  void inline add_expression(expression* expr) {
    add_expression(expression_t(expr));
  }
};



} // namespace json_path