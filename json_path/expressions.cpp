#include "expressions.hpp"
#include "json_node.hpp"
#include "json_path_filter.hpp"
#include "json_path.hpp"
#include <memory>
#include <stdexcept>


namespace json_path {

json_node_t sub_filter_expression::get_value() {
  json_node_t node = outer_expr->get_value();

  filter_expression expression(node, node, inner_expr->get_value()->as<std::string>());
  return expression.parse();
}

json_node_t wildcard_expression::get_value() {
  try {
    json_node_t retval =  json_path(*m_provider->get_current_node()).wildcard().get_current_node();
    if ((retval->type != ARRAY) || !(retval->as<std::vector<json_node>>().empty())) {
      return retval;
    }
    // else return empty.
  } catch (std::out_of_range) {} catch (std::bad_variant_access) {}
  return std::make_shared<json_node>(std::monostate());
}

} // namespace json_path
