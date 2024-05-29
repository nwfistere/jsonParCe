#include "expressions.hpp"
#include "json_path_filter.hpp"
#include <memory>
#include <stdexcept>


namespace json_path {

json_node sub_filter_expression::eval() {
  literal_value_expression* literal_expr = dynamic_cast<literal_value_expression*>(right.get());
  if (!literal_expr) {
    throw std::invalid_argument("failed to cast expr");
  }

  json_node node = left->eval();

  std::shared_ptr<json_node> shared_node = std::make_shared<json_node>(node.value);

  filter_expression expression(
    shared_node,
    shared_node,
    literal_expr->eval().as<std::string>()
  );
  return expression.parse();
}

} // namespace json_path
