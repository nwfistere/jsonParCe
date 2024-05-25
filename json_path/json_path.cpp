#include "json_path.hpp"
#include <stdexcept>

namespace json_path {

JSON_PATH_TYPE get_value_type(const value_t& value) {
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

json_node::json_node(const json_path& path) {
  type = path.get_current_node()->type;
  value = path.get_current_node()->value;
};


int on_object_value(json_parce *parser, const char *key, size_t key_length, JSON_TYPE type, const char *value, size_t value_length) {
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

int on_array_value(json_parce *parser, unsigned int index, JSON_TYPE type, const char *value, size_t value_length) {
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

json_node* parse_json(const std::string& data) {
  static json_parce_callbacks cbs = {
    .on_object_key_value_pair = on_object_value,
    .on_array_value = on_array_value
  };

  json_parce parser;
  json_node_context context = { 0 };
  
  json_parce_init(&parser);
  parser.data = (void*) &context;
  context.cb = parse_json;
  
  size_t retval = json_parce_execute(&parser, &cbs, data.c_str(), data.length());

  if (retval != data.length()) {
    throw std::runtime_error("Failed to parse json.");
  }

  return context.current_parent;
}

}




