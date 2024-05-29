#include "json_path.hpp"
#include <algorithm>
#include <stdexcept>

namespace json_path {

JSON_PATH_TYPE get_value_type(const value_t& value) {
  if (std::holds_alternative<json_parce_int_t>(value)) {
    return INT_TYPE;
  } else if (std::holds_alternative<json_parce_real_t>(value)) {
    return REAL_TYPE;
  } else if (std::holds_alternative<bool>(value)) {
    return BOOL_TYPE;
  } else if (std::holds_alternative<json_null>(value)) {
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

int on_object_value(json_parce *parser, const char *key, size_t key_length, JSON_TYPE type, const char *value, size_t value_length) {
  json_node_context* context = (json_node_context*)parser->data;
  if (!context->current_parent) {
    context->current_parent = new json_node(std::map<std::string, json_node>());
  }
  std::map<std::string, json_node>& par_val = std::get<std::map<std::string, json_node>>(context->current_parent->value);

  switch(type) {
    case(JSON_TYPE::ARRAY):
    case(JSON_TYPE::OBJECT): {
      if (value_length == 2) {
        // empty array or object...
        par_val.insert({
          std::string(key, key_length),
          (type == JSON_TYPE::OBJECT ? json_node(std::map<std::string, json_node>()) : json_node(std::vector<json_node>()))
        });
        break;
      }
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
      par_val.insert({ std::string(key, key_length), json_node(json_null()) });
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
      if (value_length == 2) {
        // empty array or object...
        par_val.push_back(type == JSON_TYPE::OBJECT ? json_node(std::map<std::string, json_node>()) : json_node(std::vector<json_node>()));
        break;
      }
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
      par_val.push_back(json_node(json_null()));
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

int json_path::normalize(int i, int len) {
  return ((i >= 0) ? i : (len + i));
}

void json_path::bounds(int start, int end, int step, int len, int& lower, int& upper) {
  int n_start = normalize(start, len);
  int n_end = normalize(end, len);

  if (step >= 0) {
    lower = std::min(std::max(n_start, 0), len);
    upper = std::min(std::max(n_end, 0), len);
  } else {
    upper = std::min(std::max(n_start, -1), len - 1);
    lower = std::min(std::max(n_end, -1), len - 1);
  }
}

json_path json_path::slice(const int* start, const int* end, const int* step) {
  int upper, lower, lstep, lend, lstart;
  std::vector<json_node> array = get<std::vector<json_node>>();

  lstep = (step ? *step : 1);
  lend = (end ? *end : (lstep >= 0 ? array.size() : ((-1 * array.size()) - 1)));
  lstart = (start ? *start : (lstep >= 0 ? 0 : (array.size() - 1)));

  std::vector<json_node> retval;
  // set bounds
  bounds(lstart, lend, lstep, array.size(), lower, upper);

  // step of 0 returns an empty list.
  if (lstep > 0) {
    for (int i = lower; i < upper; i += lstep) {
      retval.push_back(array[i]);
    }
  } else if (lstep < 0) {
    for (int i = upper; i > lower; i += lstep) {
      retval.push_back(array[i]);
    }
  }

  return json_path(json_node(retval));
}

json_path json_path::wildcard() {
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

json_path_descendant json_path::descendant() {
  auto retNodes = std::vector<json_node>();
  if (m_current_node->type == JSON_PATH_TYPE::OBJECT) {
    std::map<std::string, json_node> object = get<std::map<std::string, json_node>>();
    for (auto& pair : object) {
      std::vector<json_node> value_path_nodes = json_path(pair.second).get_descendants();
      for (auto& item : value_path_nodes) {
        retNodes.push_back(item);
      }
    }
  } else if (m_current_node->type == JSON_PATH_TYPE::ARRAY) {
    std::vector<json_node> array = get<std::vector<json_node>>();
    for (auto& item : array) {
      std::vector<json_node> value_path_nodes = json_path(item).get_descendants();
      for (auto& item2 : value_path_nodes) {
        retNodes.push_back(item2);
      }
    }
  }

  return json_path_descendant(retNodes);
}

std::vector<json_node> json_path::get_descendants() {
  auto retNodes = std::vector<json_node>();
  retNodes.push_back(*m_current_node);
  if (m_current_node->type == JSON_PATH_TYPE::OBJECT) {
    std::map<std::string, json_node> object = get<std::map<std::string, json_node>>();
    for (auto& pair : object) {
      std::vector<json_node> value_path_nodes = json_path(pair.second).get_descendants();
      for (auto& item : value_path_nodes) {
        retNodes.push_back(item);
      }
    }
  } else if (m_current_node->type == JSON_PATH_TYPE::ARRAY) {
    std::vector<json_node> array = get<std::vector<json_node>>();
    for (auto& item : array) {
      std::vector<json_node> value_path_nodes = json_path(item).get_descendants();
      for (auto& item2 : value_path_nodes) {
        retNodes.push_back(item2);
      }
    }
  }

  return retNodes;
}

}




