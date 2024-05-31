#include "json_node.hpp"
#include "json_path.hpp"

namespace json_path {

json_node::json_node(const json_path &path) {
  type = path.get_current_node()->type;
  value = path.get_current_node()->value;
}

} // namespace json_path