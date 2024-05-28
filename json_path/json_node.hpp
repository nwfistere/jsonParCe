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
using value_t = std::variant<json_parce_int_t, json_parce_real_t, std::string, bool, std::vector<json_node>, std::map<std::string, json_node>, json_null, std::monostate>;

JSON_PATH_TYPE get_value_type(const value_t& value);

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
  T as() const {
    return std::get<T>(value);
  }

  json_node conjunction(const json_node& other) const {
    std::vector<json_node> retval;

    if (other.type != type) {
      return json_node(std::monostate());
    }

    switch (type) {
      case (OBJECT): {
        auto l = as<std::map<std::string, json_node>>();
        auto r = other.as<std::map<std::string, json_node>>();
        for (const auto& i : l) {
          for (const auto& j : r) {
            if (i.second && j.second) {
              // Not sure if we should be pushing back both here.
              retval.push_back(i.second);
              retval.push_back(j.second);
            }
          }
        }
        break;
      }
      case (ARRAY): {
        std::vector<json_node> l = as<std::vector<json_node>>();
        std::vector<json_node> r = other.as<std::vector<json_node>>();

        for (const auto& i : l) {
          for (const auto& j : r) {
            if (i && j) {
              retval.push_back(i);
              retval.push_back(j);
            }
          }
        }
        break;
      }
      case (NUMBER): {
        throw std::invalid_argument("Invalid type \"NUMBER\"");
        break;
      }
      case (STRING): {
        if (*this && other) {
          retval.push_back(*this);
          retval.push_back(other);
        }
        break;
      }
      case (BOOL_TYPE): {
        if (*this && other) {
          retval.push_back(*this);
          retval.push_back(other);
        }
        break;
      }
      case (NULL_TYPE): {
        retval.push_back(*this);
        retval.push_back(other);
        break;
      }
      case (INT_TYPE): {
        if (*this && other) {
          retval.push_back(*this);
          retval.push_back(other);
        }
        break;
      }
      case (REAL_TYPE): {
        if (*this && other) {
          retval.push_back(*this);
          retval.push_back(other);
        }
        break;
      }
      case (NONE): {
        std::cerr << "conjunction: returning NONE " << type << "\n";
        return json_node(std::monostate());
        break;
      }
    }
    return json_node(retval);
  }

  json_node disjunction(const json_node& other) const {
    std::vector<json_node> retval;

    if (other.type != type) {
      return json_node(std::monostate());
    }

    switch (type) {
      case (OBJECT): {
        auto l = as<std::map<std::string, json_node>>();
        auto r = other.as<std::map<std::string, json_node>>();
        for (const auto& i : l) {
          if (i.second) {
            retval.push_back(i.second);
          }
        }
        for (const auto& i : r) {
          if (i.second) {
            retval.push_back(i.second);
          }
        }
        break;
      }
      case (ARRAY): {
        std::vector<json_node> l = as<std::vector<json_node>>();
        std::vector<json_node> r = other.as<std::vector<json_node>>();

        for (const auto& i : l) {
          if (i) {
            retval.push_back(i);
          }
        }
        for (const auto& i : r) {
          if (i) {
            retval.push_back(i);
          }
        }
        break;
      }
      case (NUMBER): {
        throw std::invalid_argument("Invalid type \"NUMBER\"");
        break;
      }
      case (STRING): {
        if (*this) {
          retval.push_back(*this);
        }
        if (other) {
          retval.push_back(other);
        }
        break;
      }
      case (BOOL_TYPE): {
        if (*this) {
          retval.push_back(*this);
        }
        if (other) {
          retval.push_back(other);
        }
        break;
      }
      case (NULL_TYPE): {
        retval.push_back(*this);
        retval.push_back(other);
        break;
      }
      case (INT_TYPE): {
        if (*this) {
          retval.push_back(*this);
        }
        if (other) {
          retval.push_back(other);
        }
        break;
      }
      case (REAL_TYPE): {
        if (*this) {
          retval.push_back(*this);
        }
        if (other) {
          retval.push_back(other);
        }
        break;
      }
      case (NONE): {
        std::cerr << "conjunction: returning NONE " << type << "\n";
        return json_node(std::monostate());
        break;
      }
    }
    return json_node(retval);
  }

  operator bool() const {
    switch (type) {
      case (OBJECT): {
        return !(as<std::map<std::string, json_node>>().empty());
      }
      case (ARRAY): {
        return !(as<std::vector<json_node>>().empty());
      }
      case (NUMBER): {
        throw std::invalid_argument("Invalid type \"NUMBER\"");
      }
      case (STRING): {
        return !(as<std::string>().empty());
      }
      case (BOOL_TYPE): {
        return as<bool>();
      }
      case (NULL_TYPE): {
        return false; // TODO: Not sure if we should return true or false for null.
      }
      case (INT_TYPE): {
        return as<json_parce_int_t>();
      }
      case (REAL_TYPE): {
        return as<json_parce_real_t>();
      }
      case (NONE): {
        return false;
      }
    }
    return true;
  }

  bool operator&&(const json_node& other) const {
    if (other.type != type) {
      return false;
    }

    if (type == JSON_PATH_TYPE::ARRAY) {
      std::vector<json_node> l = as<std::vector<json_node>>();
      std::vector<json_node> r = other.as<std::vector<json_node>>();

      for (const auto& i : l) {
        for (const auto& j : r) {
          if (i && j) {
            return true;
          }
        }
      }
      return false;
    } else if (type == JSON_PATH_TYPE::OBJECT) {
      auto l = as<std::map<std::string, json_node>>();
      auto r = other.as<std::map<std::string, json_node>>();

      for (const auto& i : l) {
        for (const auto& j : r) {
          if (i.second && j.second) {
            return true;
          }
        }
      }
      return false;
    } else if (type == JSON_PATH_TYPE::BOOL_TYPE) {
      auto l = as<bool>();
      auto r = other.as<bool>();
      return l && r;
    } else if (type == JSON_PATH_TYPE::STRING) {
      auto l = as<std::string>();
      auto r = other.as<std::string>();
      return (!(l.empty()) && !(r.empty()));
    } else if (type == JSON_PATH_TYPE::NONE) {
      std::cerr << "WARNING: compaing two invalid types.\n";
      return true;
    } else if (type == JSON_PATH_TYPE::INT_TYPE) {
      auto l = as<json_parce_int_t>();
      auto r = other.as<json_parce_int_t>();
      return (l && r);
    } else if (type == JSON_PATH_TYPE::REAL_TYPE) {
      auto l = as<json_parce_real_t>();
      auto r = other.as<json_parce_real_t>();
      return l && r;
    } else {
      std::cerr << "operator&&: defaulting to true for type " << type << "\n";
    }

    return true;
  }

  bool operator||(const json_node& other) const {
    if (other.type != type) {
      return false;
    }

    if (type == JSON_PATH_TYPE::BOOL_TYPE) {
      auto l = as<bool>();
      auto r = other.as<bool>();
      return (l || r);
    } else if (type == JSON_PATH_TYPE::STRING) {
      auto l = as<std::string>();
      auto r = other.as<std::string>();
      // not sure if this is correct...
      return (!(l.empty()) || !(r.empty()));
    } else if (type == JSON_PATH_TYPE::INT_TYPE) {
      auto l = as<json_parce_int_t>();
      auto r = other.as<json_parce_int_t>();
      return (l || r);
    } else if (type == JSON_PATH_TYPE::REAL_TYPE) {
      auto l = as<json_parce_real_t>();
      auto r = other.as<json_parce_real_t>();
      return (l || r);
    } else {
      std::cerr << "operator||: defaulting to true for type " << type << "\n";
    }

    return true;
  }

  bool operator==(const json_node& other) const {
    if (this == &other) {
      return true;
    }

    if (type != other.type) {
      return false;
    }

    switch (type) {
      case (OBJECT): {
        auto o = other.as<std::map<std::string, json_node>>();
        for (const auto& pair : as<std::map<std::string, json_node>>()) {
          if (auto search = o.find(pair.first) != o.end()) {
            if(search != pair.second) {
              return false;
            }
          } else {
            return false;
          }
        }
      }
      case (ARRAY): {
        std::vector<json_node> l = as<std::vector<json_node>>();
        std::vector<json_node> r = other.as<std::vector<json_node>>();
        if (l.size() != r.size()) {
          return false;
        }

        for (const auto& i : l) {
          if(std::find(r.begin(), r.end(), i) == std::end(r)) {
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
        std::cerr << "==: returning true for " << type << "\n";
        break;
      }
    }
    return true;
  }

  bool operator!=(const json_node& other) const {
    return !(*this == other);
  }

  bool operator>(const json_node& other) const {
    if (type != other.type) {
      throw std::invalid_argument("Invalid type comparison.");
    }

    if (type == NONE) {
      throw std::invalid_argument("comparison of NONE type.");
    }

    if ((type == OBJECT) || (type == ARRAY) || (type == BOOL_TYPE) || (type == NULL_TYPE)) {
      return false; // These types do not offer comparisons.
    }

    switch(type) {
      case(STRING): {
        return (as<std::string>() > other.as<std::string>());
      }
      case(INT_TYPE): {
        return (as<json_parce_int_t>() > other.as<json_parce_int_t>());
      }
      case(REAL_TYPE): {
        return (as<json_parce_real_t>() > other.as<json_parce_real_t>());
      }
    }

    throw std::invalid_argument("unknown type comparison " + std::to_string(type));
  }

  bool operator>=(const json_node& other) const {
    return ((*this == other) || (*this > other));
  }

  bool operator<(const json_node& other) const {
    if (type != other.type) {
      throw std::invalid_argument("Invalid type comparison.");
    }

    if (type == NONE) {
      throw std::invalid_argument("comparison of NONE type.");
    }

    if ((type == OBJECT) || (type == ARRAY) || (type == BOOL_TYPE) || (type == NULL_TYPE)) {
      return false; // These types do not offer comparisons.
    }

    switch(type) {
      case(STRING): {
        return (as<std::string>() < other.as<std::string>());
      }
      case(INT_TYPE): {
        return (as<json_parce_int_t>() < other.as<json_parce_int_t>());
      }
      case(REAL_TYPE): {
        return (as<json_parce_real_t>() < other.as<json_parce_real_t>());
      }
    }

    throw std::invalid_argument("unknown type comparison " + std::to_string(type));
  }

  bool operator<=(const json_node& other) const {
    return ((*this == other) || (*this < other));
  }
};

using json_node_t = std::shared_ptr<json_node>;

}