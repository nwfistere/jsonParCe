#pragma once

#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

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
struct json_path;
struct json_null {};
using value_t =
    std::variant<json_parce_int_t, json_parce_real_t, std::string, bool,
                 std::vector<json_node>, std::map<std::string, json_node>,
                 json_null, std::monostate>;

JSON_PATH_TYPE get_value_type(const value_t &value);

struct json_node {

  JSON_PATH_TYPE type;
  value_t value;

  json_node(const json_node &other) : value(other.value), type(other.type) {}

  json_node(const json_path &path);

  json_node(const value_t &value) : value(value), type(get_value_type(value)) {}

  ~json_node() {}

  template <typename T> T as() const { return std::get<T>(value); }

  operator bool() const {
    if (type == NONE) {
      return false;
    }
    return true;
  }

  bool operator&&(const json_node &other) const {
    return ((bool)*this && (bool)other);
  }

  bool operator||(const json_node &other) const {
    return ((bool)*this || (bool)other);
  }

  bool operator==(const json_node &other) const {
    if (this == &other) {
      return true;
    }

    if (type != other.type) {
      return false;
    }

    switch (type) {
    case (OBJECT): {
      auto o = other.as<std::map<std::string, json_node>>();
      for (const auto &pair : as<std::map<std::string, json_node>>()) {
        if (auto search = o.find(pair.first) != o.end()) {
          if (search != pair.second) {
            return false;
          }
        } else {
          return false;
        }
      }
      break;
    }
    case (ARRAY): {
      std::vector<json_node> l = as<std::vector<json_node>>();
      std::vector<json_node> r = other.as<std::vector<json_node>>();
      if (l.size() != r.size()) {
        return false;
      }

      for (const auto &i : l) {
        if (std::find(r.begin(), r.end(), i) == std::end(r)) {
          return false;
        }
      }
      break;
    }
    case (NUMBER): {
      throw std::invalid_argument("Invalid type \"NUMBER\"");
    }
    case (STRING): {
      return as<std::string>() == other.as<std::string>();
    }
    case (BOOL_TYPE): {
      return as<bool>() == other.as<bool>();
    }
    case (NULL_TYPE): {
      return true;
    }
    case (INT_TYPE): {
      return as<json_parce_int_t>() == other.as<json_parce_int_t>();
    }
    case (REAL_TYPE): {
      return as<json_parce_real_t>() == other.as<json_parce_real_t>();
    }
    case (NONE): {
      return true;
    }
    default: {
      std::cerr << "==: returning false for " << type << "\n";
      return false;
    }
    }
    return true;
  }

  bool operator!=(const json_node &other) const { return !(*this == other); }

  bool operator>(const json_node &other) const {
    if (type != other.type &&
        !(((type == INT_TYPE) && (other.type == REAL_TYPE)) ||
          ((type == REAL_TYPE) && (other.type == INT_TYPE)))) {
      return false;
    }

    if (type == NONE) {
      throw std::invalid_argument("comparison of NONE type.");
    }

    if ((type == OBJECT) || (type == ARRAY) || (type == BOOL_TYPE) ||
        (type == NULL_TYPE)) {
      return false; // These types do not offer comparisons.
    }

    switch (type) {
    case (STRING): {
      return (as<std::string>() > other.as<std::string>());
    }
    case (INT_TYPE): {
      if (other.type == INT_TYPE) {
        return (as<json_parce_int_t>() > other.as<json_parce_int_t>());
      } else {
        return (as<json_parce_int_t>() > other.as<json_parce_real_t>());
      }
    }
    case (REAL_TYPE): {
      return (as<json_parce_real_t>() > other.as<json_parce_real_t>());
    }
    default: {
      throw std::invalid_argument("unknown type comparison " +
                                  std::to_string(type));
    }
    }
  }

  bool operator>=(const json_node &other) const {
    return ((*this == other) || (*this > other));
  }

  bool operator<(const json_node &other) const {
    if (type != other.type) {
      return false;
    }

    if (type == NONE) {
      throw std::invalid_argument("comparison of NONE type.");
    }

    if ((type == OBJECT) || (type == ARRAY) || (type == BOOL_TYPE) ||
        (type == NULL_TYPE)) {
      return false; // These types do not offer comparisons.
    }

    switch (type) {
    case (STRING): {
      return (as<std::string>() < other.as<std::string>());
    }
    case (INT_TYPE): {
      return (as<json_parce_int_t>() < other.as<json_parce_int_t>());
    }
    case (REAL_TYPE): {
      return (as<json_parce_real_t>() < other.as<json_parce_real_t>());
    }
    default: {
      throw std::invalid_argument("unknown type comparison " +
                                  std::to_string(type));
    }
    }
  }

  bool operator<=(const json_node &other) const {
    return ((*this == other) || (*this < other));
  }
};

using json_node_t = std::shared_ptr<json_node>;

} // namespace json_path