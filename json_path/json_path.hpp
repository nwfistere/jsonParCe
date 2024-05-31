#pragma once

#ifndef JSON_PARCE_JSON_PATH_H
#define JSON_PARCE_JSON_PATH_H

#include "json_node.hpp"
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace json_path {

json_node* parse_json(const std::string &data);

class json_path_exception : public std::runtime_error {
public:
  json_path_exception(const std::string &msg) : runtime_error(msg) {}
};

struct json_node_context {
  json_node *current_parent;
  std::function<json_node *(const std::string &)> cb;
};

class json_path_descendant;

class json_path {
protected:
  json_node_t m_current_node;
  int normalize(int i, int len);
  void bounds(int start, int end, int step, int len, int &lower, int &upper);

public:
  json_path() = default;
  explicit json_path(const std::string &json)
      : m_current_node(parse_json(json)) {}
  explicit json_path(json_node json)
      : m_current_node(std::make_shared<json_node>(json)) {}
  virtual ~json_path() = default;

  virtual json_path select_child(const char *segment);

  json_path select_child(size_t segment);

  void select_children_in_list(json_path &node, std::size_t index);

  template <class... Indexes>
  void select_children_in_list(json_path &node, std::size_t index,
                               Indexes... indexes) {
    try {
      node.get<std::vector<json_node>>().push_back(
          *select_child(index).get_current_node());
    } catch (const std::bad_variant_access &) {
      throw json_path_exception("invalid type");
    }
    select_children_in_list(node, indexes...);
  }

  template <class... Indexes>
  json_path select_multiple_children(Indexes... indexes) {
    json_path retpath((json_node((std::vector<json_node>()))));
    select_children_in_list(retpath, indexes...);
    return retpath;
  }

  virtual json_path wildcard();

  // This is actually ..['*']
  json_path_descendant descendant();

  std::vector<json_node> get_descendants();

  json_path operator[](const std::string &segment) {
    return select_child(segment.c_str());
  }

  json_path operator[](size_t segment) { return select_child(segment); };

  json_path operator()(const std::string &segment) {
    return select_child(segment.c_str());
  }

  json_path operator()(const char *segment) { return select_child(segment); }

  json_path operator()(char c) {
    // right now only expect the wildcard.
    if (c != '*') {
      throw json_path_exception("invalid operand (expected '*')");
    }
    return wildcard();
  }

  template <class... Indexes> json_path operator()(Indexes... indexes) {
    return select_multiple_children(indexes...);
  };

  template <typename T> T &get() { return std::get<T>(m_current_node->value); }

  json_node_t get_current_node() const { return m_current_node; }

  json_node_t filter(const std::string &selector);

  json_path slice(const int *lower = nullptr, const int *upper = nullptr,
                  const int *step = nullptr);
};

class json_path_descendant : public json_path {
public:
  json_path_descendant() = default;
  explicit json_path_descendant(const std::string &json) : json_path(json) {}
  explicit json_path_descendant(std::vector<json_node> json)
      : json_path(json_node(json)) {}
  ~json_path_descendant() override = default;

  json_path select_child(const char *segment) override;

  virtual json_path wildcard() override { return *this; }
};

} // namespace json_path

#endif // JSON_PARCE_JSON_PATH_H