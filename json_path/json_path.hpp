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

template<typename T>
using nodelist = std::vector<T>;

struct json_node;

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

using value_t = std::variant<json_parce_int_t, json_parce_real_t, std::string, bool, std::monostate, std::vector<json_node>, std::map<std::string, json_node>>;

static JSON_PATH_TYPE get_value_type(const value_t& value) {
  if (std::holds_alternative<json_parce_int_t>(value)) {
    return INT_TYPE;
  } else if (std::holds_alternative<json_parce_real_t>(value)) {
    return REAL_TYPE;
  } else if (std::holds_alternative<bool>(value)) {
    return BOOL_TYPE;
  } else if (std::holds_alternative<std::monostate>(value)) {
    return NULL_TYPE;
  } else if (std::holds_alternative<std::string>(value)) {
    return STRING;
  } else if (std::holds_alternative<std::vector<json_node>>(value)) {
    return ARRAY;
  } else if (std::holds_alternative<std::map<std::string, json_node>>(value)) {
    return OBJECT;
  } else {
    return NONE;
  }
}

class json_path;

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
};

struct json_node_context {
  json_node* current_parent;
  std::function<json_node*(const std::string&)> cb;
};

static int on_array_value(json_parce *parser, unsigned int index, JSON_TYPE type, const char *value, size_t value_length) {
  json_node_context* context = (json_node_context*)parser->data;
  if (!context->current_parent) {
    context->current_parent = new json_node(std::vector<json_node>());
  }
  std::vector<json_node>& par_val = std::get<std::vector<json_node>>(context->current_parent->value);

  switch(type) {
    case(JSON_TYPE::OBJECT):
    case(JSON_TYPE::ARRAY): {
      par_val.push_back(*context->cb(std::string(value, value_length)));
      break;
    }
    case(JSON_TYPE::NUMBER): {
      json_parce_int_t rint;
      json_parce_real_t real;
      int ret = json_parce_int(value, value_length, &rint);
      if (ret != 0) {
        ret = json_parce_real(value, value_length, &real);
        if (ret != 0) {
          throw std::out_of_range("number value was out of range");
        }
        par_val.push_back(json_node(real));
      } else {
        par_val.push_back(json_node(rint));
      }
      break;
    }
    case(JSON_TYPE::STRING): {
      par_val.push_back(json_node(json_parce_string(value, value_length)));
      break;
    }
    case(JSON_TYPE::BOOL_TYPE): {
      par_val.push_back(json_node(json_parce_bool(value)));
      break;
    }
    case(JSON_TYPE::NULL_TYPE): {
      par_val.push_back(json_node(std::monostate()));
      break;
    }
    default: {
      throw std::out_of_range("Invalid json type");
      break;
    }
  }

  return 0;
}

static int on_object_value(json_parce *parser, const char *key, size_t key_length, JSON_TYPE type, const char *value, size_t value_length) {
  json_node_context* context = (json_node_context*)parser->data;
  if (!context->current_parent) {
    context->current_parent = new json_node(std::map<std::string, json_node>());
  }
  std::map<std::string, json_node>& par_val = std::get<std::map<std::string, json_node>>(context->current_parent->value);

  switch(type) {
    case(JSON_TYPE::ARRAY):
    case(JSON_TYPE::OBJECT): {
      json_node* node = context->cb(std::string(value, value_length));
      par_val.insert({ std::string(key, key_length), *node});
      break;
    }
    case(JSON_TYPE::NUMBER): {
      json_parce_int_t rint;
      json_parce_real_t real;
      int ret = json_parce_int(value, value_length, &rint);
      if (ret != 0) {
        ret = json_parce_real(value, value_length, &real);
        if (ret != 0) {
          throw std::out_of_range("number value was out of range");
        }
        par_val.insert({ std::string(key, key_length), json_node(real) });
      } else {
        par_val.insert({ std::string(key, key_length), json_node(rint) });
      }
      break;
      
      break;
    }
    case(JSON_TYPE::STRING): {
      par_val.insert({ std::string(key, key_length), json_node(std::string(value, value_length)) });
      break;
    }
    case(JSON_TYPE::BOOL_TYPE): {
      par_val.insert({ std::string(key, key_length), json_node(json_parce_bool(value)) });
      break;
    }
    case(JSON_TYPE::NULL_TYPE): {
      par_val.insert({ std::string(key, key_length), json_node(std::monostate()) });
      break;
    }
    default: {
      throw std::out_of_range("Invalid json type");
      break;
    }
  }

  return 0;
}

static json_parce_callbacks cbs = {
  .on_object_key_value_pair = on_object_value,
  .on_array_value = on_array_value
};

static json_node* parse_json(const std::string& data) {
  json_parce parser;
  json_node_context context = { 0 };
  json_parce_init(&parser);
  parser.data = (void*) &context;
  context.cb = parse_json;
  size_t retval = json_parce_execute(&parser, &cbs, data.c_str(), data.length());

  return context.current_parent;
}

// class base_json_path_node {
// protected:
//   JSON_PATH_TYPE NodeEnum;

// public:
//   base_json_path_node() = delete;
//   base_json_path_node(const base_json_path_node& other) = delete;
//   base_json_path_node(base_json_path_node&& other) = delete;
//   base_json_path_node& operator=(const base_json_path_node& other) = delete;
//   base_json_path_node& operator=(base_json_path_node&& other) = delete;

//   explicit base_json_path_node(JSON_PATH_TYPE type) : NodeEnum(type) {}
//   virtual ~base_json_path_node();

//   JSON_PATH_TYPE type() const {
//     return NodeEnum;
//   }
// };

// template<JSON_PATH_TYPE Path_Type, typename CXXType>
// class json_path_node : public base_json_path_node {
// private:
//   json_path_node() : base_json_path_node(Path_Type) {}
//   std::shared_ptr<json_node> m_node;

// public:
//   using NodeType = CXXType;

//   json_path_node(json_node* node) : json_path_node(), m_node(node) {}
//   json_path_node(std::shared_ptr<json_node> node) : json_path_node(), m_node(node) {}

//   ~json_path_node() override = default;

//   CXXType get() {
//     std::get<CXXType>(m_node->value);
//   }

// };

class json_path {
private:
  std::shared_ptr<json_node> m_current_node;
public:
  json_path() = default;
  explicit json_path(const std::string& json) : m_current_node(parse_json(json)) {}
  explicit json_path(json_node& json) : m_current_node(std::make_unique<json_node>(json)) {}
  ~json_path() = default;

  json_path operator[](const std::string& selector) {
    return (*this)(selector);
  }

  // json_path operator[](const char* selector) {
  //   return (*this)(selector);
  // };

  json_path operator[](size_t index) {
    return (*this)(index);
  };

  json_path operator()(const std::string& selector) {
    return (*this)(selector.c_str());
  }

  json_path operator()(const char* selector) {
    std::cout << "get child key \"" << selector << "\"\n";
    try {
      return json_path(std::get<std::map<std::string, json_node>>(m_current_node->value).at(selector));
    } catch (const std::bad_variant_access&) {
      throw json_path_exception("invalid type");
    } catch (const std::out_of_range&) {
      throw json_path_exception("Object does not contain key" + std::string(selector));
    }
    return *this; // return a child or something
  }

  json_path operator()(size_t index) {
    std::cout << "get index \"" << index << "\"\n";
    try {
      return json_path(std::get<std::vector<json_node>>(m_current_node->value).at(index));
    } catch (const std::bad_variant_access&) {
      throw json_path_exception("invalid type");
    } catch (const std::out_of_range&) {
      throw json_path_exception("Object does not contain index" + std::to_string(index));
    }
    return *this; 
  }

  template<class... Indexes>
  json_path operator()(size_t index, Indexes... indexes) {
    this->operator()(index);
    return this->operator()(std::forward<Indexes>(indexes)...);
  };

  std::vector<json_node> get() {
    if (std::holds_alternative<std::vector<json_node>>(m_current_node->value)) {
      return std::get<std::vector<json_node>>(m_current_node->value);
    }

    std::vector<json_node> retval;
    retval.push_back(m_current_node->value);
    return retval;
  }

  json_node get() const {
    return m_current_node->value;
  }
};

} // json_path

#endif // JSON_PARCE_JSON_PATH_H