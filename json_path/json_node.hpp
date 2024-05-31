#pragma once

#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>
#include <memory>

#include "json_parce.h"

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
struct json_null {};
using value_t =
    std::variant<json_parce_int_t, json_parce_real_t, std::string, bool,
                 std::vector<json_node>, std::map<std::string, json_node>,
                 json_null, std::monostate>;

JSON_PATH_TYPE get_value_type(const value_t &value);

struct json_node {

  JSON_PATH_TYPE type;
  value_t value;

  json_node(const json_node &other) : type(other.type), value(other.value) {}

  json_node(const json_path &path);

  json_node(const value_t &value) : type(get_value_type(value)), value(value) {}

  ~json_node() {}

  template <typename T> T as() const { return std::get<T>(value); }

  operator bool() const;
  bool operator&&(const json_node &other) const;
  bool operator||(const json_node &other) const;
  bool operator==(const json_node &other) const;
  bool operator!=(const json_node &other) const;
  bool operator>(const json_node &other) const;
  bool operator>=(const json_node &other) const;
  bool operator<(const json_node &other) const;
  bool operator<=(const json_node &other) const;
};

using json_node_t = std::shared_ptr<json_node>;

} // namespace json_path