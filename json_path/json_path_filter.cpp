#include "json_path_filter.hpp"
#include "expressions.hpp"
#include <memory>
#include <stdexcept>

namespace json_path {

void expression_parser::cleanup_expressions(bool final) {
  // At this point, Everything should be
  // parsed in this context, combine the expressions.
  if (expressions.size() <= 1) {
    return;
  }

  size_t virtual_start = 0;
  for (size_t i = virtual_start; i < expressions.size(); i++) {
    expression *item = expressions[i].get();
    multi_filter_wrapper_expression *expr = nullptr;
    if (((expr = dynamic_cast<multi_filter_wrapper_expression *>(item)) !=
         nullptr)) {
      if (virtual_start != i) {
        throw std::invalid_argument("virtual_start should always == i: " +
                                    std::to_string(virtual_start) +
                                    std::string(" != ") + std::to_string(i));
      }
      virtual_start++;
    }
  }

  for (size_t i = virtual_start; i < expressions.size(); i++) {
    expression *item = expressions[i].get();
    not_expression *expr = nullptr;
    if (((expr = dynamic_cast<not_expression *>(item)) != nullptr) &&
        !(expr->expr)) {
      expr->expr = expressions[i + 1];
      expressions.erase(expressions.begin() + i + 1);
      i--; // May need to goto restart here...
    }
  }

  for (size_t i = virtual_start; i < expressions.size(); i++) {
    expression *item = expressions[i].get();
    equivalent_expression *e_expr = nullptr;
    not_equivalent_expression *ne_expr = nullptr;
    lt_expression *lt_expr = nullptr;
    le_expression *le_expr = nullptr;
    gt_expression *gt_expr = nullptr;
    ge_expression *ge_expr = nullptr;
    if (((e_expr = dynamic_cast<equivalent_expression *>(item)) != nullptr) &&
        !(e_expr->left) && !(e_expr->right)) {
      e_expr->left = expressions[i - 1];
      e_expr->right = expressions[i + 1];
      expressions.erase(expressions.begin() + i + 1);
      expressions.erase(expressions.begin() + i - 1);
      i -= 2; // May need to goto restart here...
    } else if (((ne_expr = dynamic_cast<not_equivalent_expression *>(item)) !=
                nullptr) &&
               !(ne_expr->left) && !(ne_expr->right)) {
      ne_expr->left = expressions[i - 1];
      ne_expr->right = expressions[i + 1];
      expressions.erase(expressions.begin() + i + 1);
      expressions.erase(expressions.begin() + i - 1);
      i -= 2; // May need to goto restart here...
    } else if (((lt_expr = dynamic_cast<lt_expression *>(item)) != nullptr) &&
               !(lt_expr->left) && !(lt_expr->right)) {
      lt_expr->left = expressions[i - 1];
      lt_expr->right = expressions[i + 1];
      expressions.erase(expressions.begin() + i + 1);
      expressions.erase(expressions.begin() + i - 1);
      i -= 2; // May need to goto restart here...
    } else if (((le_expr = dynamic_cast<le_expression *>(item)) != nullptr) &&
               !(le_expr->left) && !(le_expr->right)) {
      le_expr->left = expressions[i - 1];
      le_expr->right = expressions[i + 1];
      expressions.erase(expressions.begin() + i + 1);
      expressions.erase(expressions.begin() + i - 1);
      i -= 2; // May need to goto restart here...
    } else if (((gt_expr = dynamic_cast<gt_expression *>(item)) != nullptr) &&
               !(gt_expr->left) && !(gt_expr->right)) {
      gt_expr->left = expressions[i - 1];
      gt_expr->right = expressions[i + 1];
      expressions.erase(expressions.begin() + i + 1);
      expressions.erase(expressions.begin() + i - 1);
      i -= 2; // May need to goto restart here...
    } else if (((ge_expr = dynamic_cast<ge_expression *>(item)) != nullptr) &&
               !(ge_expr->left) && !(ge_expr->right)) {
      ge_expr->left = expressions[i - 1];
      ge_expr->right = expressions[i + 1];
      expressions.erase(expressions.begin() + i + 1);
      expressions.erase(expressions.begin() + i - 1);
      i -= 2; // May need to goto restart here...
    }
  }

  for (size_t i = virtual_start; i < expressions.size(); i++) {
    expression *item = expressions[i].get();
    and_expression *expr = nullptr;
    if (((expr = dynamic_cast<and_expression *>(item)) != nullptr) &&
        !(expr->left) && !(expr->right)) {
      expr->left = expressions[i - 1];
      expr->right = expressions[i + 1];
      expressions.erase(expressions.begin() + i + 1);
      expressions.erase(expressions.begin() + i - 1);
      i -= 2; // May need to goto restart here...
    }
  }

  for (size_t i = virtual_start; i < expressions.size(); i++) {
    expression *item = expressions[i].get();
    or_expression *expr = nullptr;
    if (((expr = dynamic_cast<or_expression *>(item)) != nullptr) &&
        !(expr->left) && !(expr->right)) {
      expr->left = expressions[i - 1];
      expr->right = expressions[i + 1];
      expressions.erase(expressions.begin() + i + 1);
      expressions.erase(expressions.begin() + i - 1);
      i -= 2; // May need to goto restart here...
    }
  }

  if (final) {
    if (expressions.size() > (virtual_start + 1)) {
      throw std::invalid_argument("expressions is still too large! " +
                                  std::to_string(expressions.size()));
    }
    if (virtual_start) {
      // It's a multi-filter, put the last filter in a wrapper.
      expression_t back = expressions.back();
      expressions.pop_back();
      add_expression(std::make_shared<multi_filter_wrapper_expression>(back));
    }
  }
}

std::string expression_parser::get_selector(std::string str) {
  // TODO: Don't worry about '\'', '"', or '[' yet...
  size_t i = 0;
  while (str[i++]) {
    switch (str[i]) {
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

std::vector<expression_t> expression_parser::parse(std::string str) {
  size_t i = 0;
  for (i = 0; i < str.size(); i++) {
    char c = str[i];
    char p = str[i + 1]; // peek
    switch (c) {
    case '(': {
      try {
        std::string substring =
            get_full_parentheses_statement(str.substr(i + 1));
        expression_parser child_parser(m_provider);
        std::vector<expression_t> expressions = child_parser.parse(
            substring); // shouldn't have multiple expressions in ()
        expression_t expr =
            expression_t(new grouped_expression(expressions[0]));
        add_expression(expr);
        i += substring.size() + 1; // add 1 to get past ')'
        break;
      } catch (std::invalid_argument &e) {
        std::string what = e.what();
        what += " +" + std::to_string(i);
        throw std::invalid_argument(what);
      }
    }
    case '!': {
      if (p != '=') {
        add_expression(std::make_shared<not_expression>(nullptr));
      } else {
        add_expression(
            std::make_shared<not_equivalent_expression>(nullptr, nullptr));
      }
      break;
    }
    case '$': {
      add_expression(std::make_shared<root_node_expression>(m_provider));
      break;
    }
    case '@': {
      add_expression(std::make_shared<current_node_expression>(m_provider));
      break;
    }
    case '.': { // TODO: Don't worry about ".." yet.
      std::string child = get_selector(str.substr(i + 1));
      expression_t back = expressions.back();
      expressions.pop_back();
      value_expression_t value =
          std::dynamic_pointer_cast<value_expression>(back);
      if (child == std::string("*")) {
        add_expression(std::make_shared<selector_expression>(
            value, std::make_shared<wildcard_expression>(m_provider)));
      } else {
        add_expression(std::make_shared<selector_expression>(
            value, std::make_shared<selector_child_expression>(
                       std::make_shared<json_node>(child))));
      }
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
      if (c == '>' && equal_to) {
        add_expression(std::make_shared<ge_expression>(nullptr, nullptr));
      } else if (c == '>') {
        add_expression(std::make_shared<gt_expression>(nullptr, nullptr));
      } else if (c == '<' && equal_to) {
        add_expression(std::make_shared<le_expression>(nullptr, nullptr));
      } else if (c == '<') {
        add_expression(std::make_shared<lt_expression>(nullptr, nullptr));
      } else {
        add_expression(
            std::make_shared<equivalent_expression>(nullptr, nullptr));
      }
      if (equal_to || c == '=') {
        i++; // skip next character.
      }
      break;
    }
    case '|': {
      if (p != '|') {
        throw std::invalid_argument("Only one | found. " + std::to_string(i));
      }
      add_expression(std::make_shared<or_expression>(nullptr, nullptr));
      i++; // skip next character.
      break;
    }
    case '&': {
      if (p != '&') {
        throw std::invalid_argument("Only one & found. " + std::to_string(i));
      }
      add_expression(std::make_shared<and_expression>(nullptr, nullptr));
      i++; // skip next character.
      break;
    }
    case '\'':
    case '"': {
      // A string literal.
      std::string substring = get_full_string(str.substr(i));
      add_expression(std::make_shared<literal_value_expression>(
          std::make_shared<json_node>(substring)));
      i += substring.size() + 1;
      break;
    }
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case '-': {
      JSON_PATH_TYPE type = NONE;
      json_parce_int_t k = 0;
      json_parce_real_t j = 0;
      size_t len;
      get_number(str.substr(i), k, j, type, len);
      if (type == INT_TYPE) {
        add_expression(std::make_shared<literal_value_expression>(
            std::make_shared<json_node>(k)));
      } else if (type == REAL_TYPE) {
        add_expression(std::make_shared<literal_value_expression>(
            std::make_shared<json_node>(j)));
      } else {
        throw std::invalid_argument("Invalid string found!");
      }
      i += len - 1; // -1 to not skip the next string.
      break;
    }
    case 'm': {
      // match function
      // example: match(@.date, "1974-05-..")
      i += 6; // get past "match("
      std::string first_param = get_function_parameter(str.substr(i));
      i += first_param.size() + 1; // add 1 to get past ','
      std::string second_param = get_function_parameter(str.substr(i));
      i += second_param.size() + 1; // add 1 to get past ')'
      expression_parser child_parser(m_provider);
      std::vector<expression_t> expressions = child_parser.parse(first_param);
      expression_t expr = std::make_shared<match_function_expression>(
          expressions[0], std::make_shared<literal_value_expression>(
                              std::make_shared<json_node>(trim(second_param))));
      add_expression(expr);
      break;
    }
    case 's': {
      // search function
      // example: search(@.b, "[jk]")
      i += 7; // get past "search("
      std::string first_param = get_function_parameter(str.substr(i));
      i += first_param.size() + 1; // add 1 to get past ','
      std::string second_param = get_function_parameter(str.substr(i));
      i += second_param.size() + 1; // add 1 to get past ')'
      expression_parser child_parser(m_provider);
      std::vector<expression_t> expressions = child_parser.parse(first_param);
      expression_t expr = std::make_shared<search_function_expression>(
          expressions[0], std::make_shared<literal_value_expression>(
                              std::make_shared<json_node>(trim(second_param))));
      add_expression(expr);
      break;
    }
    case '[': {
      // nested filter
      std::string filter = get_full_sub_filter_statement(str.substr(i + 1));
      value_expression_t back = std::dynamic_pointer_cast<value_expression>(
          expressions.back()); // TODO: Should this be done now?
      expressions.pop_back();
      add_expression(std::make_shared<sub_filter_expression>(
          back,
          std::make_shared<literal_value_expression>(
              std::make_shared<json_node>(filter)),
          m_provider));
      i += filter.size() + 1;
      break;
    }
    case ',': {
      // multiple filters
      // clean up current expressions and throw them into
      // multi_filter_wrapper_expression.
      cleanup_expressions(false);
      expression_t back = expressions.back(); // TODO: Should this be done now?
      expressions.pop_back();
      add_expression(std::make_shared<multi_filter_wrapper_expression>(back));
      break;
    }
    case '?': {
      if (dynamic_cast<multi_filter_wrapper_expression *>(
              expressions.back().get()) == nullptr) {
        throw std::invalid_argument("invalid '?' at index: " +
                                    std::to_string(i));
      }
      break;
    }
    default: {
      throw std::invalid_argument(std::string("Invalid character '") + c +
                                  std::string("' at index: ") +
                                  std::to_string(i));
    }
    }
  }
  cleanup_expressions(true);
  // TODO: verify everything is valid.
  return expressions;
}

json_node_t filter_expression::parse() {
  if (!m_expression.starts_with('?')) {
    throw std::invalid_argument("Filter doesn't start with '?'");
  }
  expression_parser parser(m_provider);
  parser.parse(m_expression.substr(1));

  json_node_t node = m_provider->get_current_node();
  std::vector<expression_t> expressions = parser.expressions;

  std::vector<json_node> retval;
  for (auto &expression : expressions) {
    switch (node->type) {
    case ARRAY: {
      for (auto &n : node->as<std::vector<json_node>>()) {
        m_provider->set_current_node(std::make_shared<json_node>(n));
        bool result = expression->eval();
        if (result) {
          retval.push_back(n);
        }
      }
      break;
    }
    case OBJECT: {
      for (auto &pair : node->as<std::map<std::string, json_node>>()) {
        m_provider->set_current_node(std::make_shared<json_node>(pair.second));
        bool result = expression->eval();
        if (result) {
          // Push back n rather than returned node.. Not sure if correct.
          retval.push_back(pair.second);
        }
      }
      break;
    }
    default: {
      return std::make_shared<json_node>(std::monostate());
    }
    }
  }
  if (retval.empty()) {
    return std::make_shared<json_node>(std::monostate());
  }
  return std::make_shared<json_node>(retval);
};

} // namespace json_path