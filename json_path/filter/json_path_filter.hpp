#pragma once

#include <stdexcept>
#include <string>
#include <vector>
#include "json_node.hpp"
#include "filter/expressions.hpp"

namespace json_path {

struct json_node;

class filter_expression {
private:
  std::string m_expression;
  node_provider_t m_provider;

public:
  filter_expression(json_node_t root, json_node_t current, const std::string& expression) : m_expression(expression), m_provider(new node_provider(root, current)) {}
  void parse();

};

static std::string get_full_parentheses_statement(const std::string& statement) {
  int level = 1;
  size_t i;
  for (i = 0; i < statement.size(); i++) {
    if (statement[i] == ')') {
      level--;
      if (level == 0) {
        break;
      }
    } else if (statement[i] == '(') {
      level++;
    }
    // TODO: string edge case
  }
  if (level != 0) {
    throw std::invalid_argument("No end of parentheses" + std::to_string(i));
  }
  return statement.substr(0, i);
}

static std::string get_full_string(const std::string& statement) {
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
    throw std::invalid_argument("No end of string" + std::to_string(i));
  }
  return statement.substr(1, i - 1);
}

struct expression_parser {
  node_provider_t m_provider;
  std::vector<expression_t> expressions;
  expression_parser(node_provider_t provider) : m_provider(provider) {}
  ~expression_parser() = default;

  void add_expression(expression_t expr) {
    if (!expressions.empty()) {
      expression_t back = expressions.back();
      binary_expression* binary_expr = dynamic_cast<binary_expression*>(back.get());

      if (binary_expr != nullptr) {
        while ((binary_expr = dynamic_cast<binary_expression*>(back.get())) != nullptr) {
          if (!binary_expr->has_right()) {
            break;
          }
          back = expression_t(binary_expr);
        }

        if (!binary_expr) {
          throw std::invalid_argument("Expected a binary expression, didn't get one.");
        } else {
          binary_expr->set_right(expr);
          return;
        }
      }
    }
    
    expressions.push_back(expr);
  }

  void add_expression(expression* expr) {
    add_expression(expression_t(expr));
  }

  std::string get_selector(std::string str) {
    // TODO: Don't worry about '\'', '"', or '[' yet...
    size_t i = 0;
    while (str[i++]) {
      switch(str[i]) {
        case ' ':
        case '.':
        case '=':
        case '>':
        case '<':
        case '&':
        case '|':
        case '(':
        case '*':
        case ',': {
          return str.substr(0, i);
        }
      }
    }
    return str;
  }

  expression_t parse(std::string str) {
    size_t i = 0;
    for (i = 0; i < str.size(); i++) {
      char c = str[i];
      char p = str[i + 1]; // peek
      switch (c) {
        case '(': {
          try {
            std::string substring = get_full_parentheses_statement(&str.data()[i + 1]);
            expression_parser child_parser(m_provider);
            expression_t returned_expression = child_parser.parse(substring.substr(0, substring.size() - 1)); // cut off last ')'
            expression_t expr = expression_t(new grouped_expression(returned_expression));
            add_expression(expr);
            i += substring.size();
            break;
          } catch (std::invalid_argument& e) {
            std::string what = e.what();
            what += " +" + std::to_string(i);
            throw std::invalid_argument(what);
          }
        }
        case '!': {
          if (p != '=') {
            add_expression(new not_expression(nullptr));
          } else {
            expression_t back = expressions.back();
            expressions.pop_back();
            add_expression(new not_equivalent_expression(back, nullptr));
          }
          break;
        }
        case '$': {
          add_expression(new root_node_expression(m_provider));
          break;
        }
        case '@': {
          add_expression(new current_node_expression(m_provider));
          break;
        }
        case '.': { //TODO: Don't worry about ".." yet.
          std::string child = get_selector(&str.data()[i+1]);
          expression_t back = expressions.back();
          expressions.pop_back();
          add_expression(new selector_expression(back, expression_t(new selector_child_expression(new json_node(child)))));
          i += child.size();
          break;
        }
        case ' ': {
          // ignore! unless bad stuff
          break;
        }
        case '=': {
          if (p != '=') {
            throw std::invalid_argument("Found singular '='");
          }
          /* fall through */
        }
        case '>':
        case '<': {
          bool equal_to = ((c == '<' || c == '>') && p == '=');
          expression_t back = expressions.back();
          expressions.pop_back();
          if (c == '>' && equal_to) {
            add_expression(new ge_expression(back, nullptr));
          } else if (c == '>') {
            add_expression(new gt_expression(back, nullptr));
          } else if (c == '<' && equal_to) {
            add_expression(new le_expression(back, nullptr));
          } else if (c == '<') {
            add_expression(new lt_expression(back, nullptr));
          } else {
            add_expression(new equivalent_expression(back, nullptr));
          }
          if (equal_to || c == '=') {
            i++; // skip next character.
          }
          break;
        }
        case '\'':
        case '"': {
          // A string literal.
          std::string substring = get_full_string(&str.data()[i]);
          add_expression(new literal_value_expression(new json_node(substring)));
          i += substring.size() + 1;
          break;
        }
        default: {
          throw std::invalid_argument(std::string("Invalid character '") + c + std::string("' at index: ") + std::to_string(i));
        }
      }
    }
    // TODO: verify everything is valid.
    return expressions[0];
  }
};

void filter_expression::parse() {
  if (!m_expression.starts_with('?')) {
    throw std::invalid_argument("Filter doesn't start with '?'");
  }
  expression_parser parser(m_provider);
  parser.parse(m_expression.substr(1));
  if (parser.expressions.size() != 1) {
    throw std::invalid_argument("Invalid number of expressions remaining, expected 1, got " + std::to_string(parser.expressions.size()));
  }

  json_node_t node = m_provider->get_current_node();
  expression_t expression = parser.expressions[0];

  std::vector<json_node> retval;
  switch (node.type) {
    case ARRAY: {
      for (const auto& n : node->as<std::vector<json_node>>()) {
        m_provider->set_current_node(json_node_t(&n));
        expression->eval()
      }
      break;
    }
    case NONE: {
      break;
    }
    default: {
      // Keep current node the same and continue on.
    }
  }
  
};

} // namespace json_path