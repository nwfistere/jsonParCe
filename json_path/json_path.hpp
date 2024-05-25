#pragma once

#include <iostream>
#include "json_parce.h"
#include <stdexcept>
#include <vector>
#include <variant>
#include <map>
#include <queue>
#include <functional>
#include <string>

#ifndef JSON_PARCE_JSON_PATH_H
#define JSON_PARCE_JSON_PATH_H

namespace json_path {


enum JSON_PATH_TYPE {
  NONE = JSON_TYPE::NONE,
  OBJECT = JSON_TYPE::OBJECT,
  ARRAY = JSON_TYPE::ARRAY,
  NUMBER = JSON_TYPE::NUMBER, // Won't use, not specific enough..
  STRING = JSON_TYPE::STRING,
  BOOL_TYPE = JSON_TYPE::BOOL_TYPE,
  NULL_TYPE = JSON_TYPE::NULL_TYPE,
  INT_TYPE,
  REAL_TYPE
};

struct json_node;
class json_path;

using value_t = std::variant<json_parce_int_t, json_parce_real_t, std::string, bool, std::monostate, std::vector<json_node>, std::map<std::string, json_node>>;

JSON_PATH_TYPE get_value_type(const value_t& value);
int on_object_value(json_parce *parser, const char *key, size_t key_length, JSON_TYPE type, const char *value, size_t value_length);
int on_array_value(json_parce *parser, unsigned int index, JSON_TYPE type, const char *value, size_t value_length);
json_node* parse_json(const std::string& data);

class json_path_exception : public std::runtime_error {
public:
  json_path_exception(const std::string& msg) : runtime_error(msg) {}
};

struct json_node {
  JSON_PATH_TYPE type;
  value_t value;

  json_node(const json_node& other) : value(other.value), type(other.type) {}

  json_node(const json_path& path);

  json_node(const value_t& value) : value(value), type(get_value_type(value)) {
    if (type == NONE) {
      throw std::out_of_range("Invalid json type");
    }
  }

  ~json_node() {
    // if object or array, delete children.
  }

  template<typename T>
  T as() {
    return std::get<T>(value);
  }
};

struct json_node_context {
  json_node* current_parent;
  std::function<json_node*(const std::string&)> cb;
};

class json_path {
private:
  std::shared_ptr<json_node> m_current_node;
public:
  json_path() = default;
  explicit json_path(const std::string& json) : m_current_node(parse_json(json)) {}
  explicit json_path(json_node json) : m_current_node(std::make_unique<json_node>(std::move(json))) {}
  ~json_path() = default;

  json_path select_child(const char* segment) {
    try {
      return json_path(std::get<std::map<std::string, json_node>>(m_current_node->value).at(segment));
    } catch (const std::bad_variant_access&) {
      throw json_path_exception("invalid type");
    } catch (const std::out_of_range&) {
      throw json_path_exception("Object does not contain key" + std::string(segment));
    }
    return *this; // return a child or something
  }

  json_path select_child(size_t segment) {
    try {
      return json_path(std::get<std::vector<json_node>>(m_current_node->value).at(segment));
    } catch (const std::bad_variant_access&) {
      throw json_path_exception("invalid type");
    } catch (const std::out_of_range&) {
      throw json_path_exception("Object does not contain index" + std::to_string(segment));
    }
    return *this; 
  }

  void select_children_in_list(json_path& node, std::size_t index) {
    try {
      node.get<std::vector<json_node>>().push_back(*select_child(index).get_current_node());
    } catch (const std::bad_variant_access&) {
      throw json_path_exception("invalid type");
    }
  }

  template<class... Indexes>
  void select_children_in_list(json_path& node, std::size_t index, Indexes... indexes) {
    try {
      node.get<std::vector<json_node>>().push_back(*select_child(index).get_current_node());
    } catch (const std::bad_variant_access&) {
      throw json_path_exception("invalid type");
    }
    select_children_in_list(node, indexes...);
  }

  template<class... Indexes>
  json_path select_multiple_children(Indexes... indexes) {
    json_path retpath((json_node((std::vector<json_node>()))));
    select_children_in_list(retpath, indexes...);
    return retpath;
  }

  json_path wildcard() {
    auto retNodes = std::vector<json_node>();
    if (m_current_node->type == JSON_PATH_TYPE::OBJECT) {
      std::map<std::string, json_node> object = std::get<std::map<std::string, json_node>>(m_current_node->value);
      for (const auto& pair : object) {
        retNodes.push_back(pair.second);
      }
    } else if (m_current_node->type == JSON_PATH_TYPE::ARRAY) {
      return (*this);
    }

    return json_path(json_node(retNodes));
  }

  json_path descendant() {
    auto retNodes = std::vector<json_node>();
  }

  json_path operator[](const std::string& segment) {
    return select_child(segment.c_str());
  }

  json_path operator[](size_t segment) {
    return select_child(segment);
  };

  json_path operator()(const std::string& segment) {
    return select_child(segment.c_str());
  }

  json_path operator()(const char* segment) {
    return select_child(segment);
  }

  json_path operator()(char c) {
    // right now only expect the wildcard.
    if (c != '*') {
      throw json_path_exception("invalid operand (expected '*')");
    }
    return wildcard();
  }

  template<class... Indexes>
  json_path operator()(Indexes... indexes) {
    return select_multiple_children(indexes...);
  };

  template<typename T>
  T& get() {
    return std::get<T>(m_current_node->value);
  }

  std::shared_ptr<json_node> get_current_node() const {
    return m_current_node;
  }
};

} // json_path

#endif // JSON_PARCE_JSON_PATH_H