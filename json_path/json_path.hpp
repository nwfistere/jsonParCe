#pragma once

#ifndef JSON_PARCE_JSON_PATH_H
#define JSON_PARCE_JSON_PATH_H

#include <cstddef>
#include "json_node.hpp"
#include <stdexcept>
#include <vector>
#include <map>
#include <functional>
#include <string>
#include <memory>

namespace json_path {

int on_object_value(json_parce *parser, const char *key, size_t key_length, JSON_TYPE type, const char *value, size_t value_length);
int on_array_value(json_parce *parser, unsigned int index, JSON_TYPE type, const char *value, size_t value_length);
json_node* parse_json(const std::string& data);

class json_path_exception : public std::runtime_error {
public:
  json_path_exception(const std::string& msg) : runtime_error(msg) {}
};

struct json_node_context {
  json_node* current_parent;
  std::function<json_node*(const std::string&)> cb;
};

class json_path_descendant;

class json_path {
protected:
  json_node_t m_current_node;
  int normalize(int i, int len);
  void bounds(int start, int end, int step, int len, int& lower, int& upper);
public:
  json_path() = default;
  explicit json_path(const std::string& json) : m_current_node(parse_json(json)) {}
  explicit json_path(json_node json) : m_current_node(std::make_shared<json_node>(json)) {}
  virtual ~json_path() = default;

  virtual json_path select_child(const char* segment) {
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

  virtual json_path wildcard();

  // This is actually ..['*']
  json_path_descendant descendant();

  std::vector<json_node> get_descendants();

  json_path filter(const std::string& expression) {

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

  json_node_t get_current_node() const {
    return m_current_node;
  }

  json_path slice(const int* lower = nullptr, const int* upper = nullptr, const int* step = nullptr);

};

class json_path_descendant : public json_path {
public:
  json_path_descendant() = default;
  explicit json_path_descendant(const std::string& json) : json_path(json) {}
  explicit json_path_descendant(std::vector<json_node> json) : json_path(json_node(json)) {}
  ~json_path_descendant() override = default;


  json_path select_child(const char* segment) override {
    auto retNodes = std::vector<json_node>();
    for (auto& child : get<std::vector<json_node>>()) {
      if (child.type == JSON_PATH_TYPE::OBJECT) {
        try {
          retNodes.push_back(std::get<std::map<std::string, json_node>>(child.value).at(segment));
        } catch (const std::out_of_range&) { /* do nothing */ }
      }
    }
    return json_path_descendant(retNodes);
  }

  virtual json_path wildcard() override {
    return *this;
  }

};


} // json_path

#endif // JSON_PARCE_JSON_PATH_H