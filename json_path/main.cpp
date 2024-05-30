#include "json_path_filter.hpp"
#include "json_node.hpp"
#include "json_path.hpp"
#include <iostream>

static void test_filter_expressions();
static void rfc_test_filter_selector();
static void to_json_array(std::ostream&, std::vector<json_path::json_node> results);
static void to_json_object(std::ostream&, std::map<std::string, json_path::json_node> results);

std::string json = R"(
{
  "stringKey": "stringValue",
  "numberKey": 12345,
  "floatKey": 123.45,
  "booleanKey": true,
  "nullKey": null,
  "arrayKey": [
    "stringValue",
    67890,
    67.89,
    false,
    null,
    {
      "nestedStringKey": "nestedStringValue",
      "nestedArrayKey": [
        "deepStringValue",
        98765,
        98.76,
        true,
        null,
        {
          "deepNestedKey": "deepNestedValue"
        }
      ]
    }
  ],
  "objectKey": {
    "innerStringKey": "innerStringValue",
    "innerNumberKey": 54321,
    "innerFloatKey": 54.32,
    "innerBooleanKey": false,
    "innerNullKey": null,
    "innerArrayKey": [
      {
        "deepInnerStringKey": "deepInnerStringValue",
        "deepInnerNumberKey": 12321,
        "deepInnerFloatKey": 12.32,
        "deepInnerBooleanKey": true,
        "deepInnerNullKey": null
      },
      "anotherString",
      11223,
      11.23,
      true
    ],
    "innerObjectKey": {
      "deepObjectStringKey": "deepObjectStringValue",
      "deepObjectNumberKey": 32123,
      "deepObjectFloatKey": 32.12,
      "deepObjectBooleanKey": false,
      "deepObjectNullKey": null,
      "deepObjectArrayKey": [
        "deepArrayString",
        23232,
        23.23,
        false,
        null
      ],
      "deepObjectObjectKey": {
        "veryDeepStringKey": "veryDeepStringValue",
        "veryDeepNumberKey": 43234,
        "veryDeepFloatKey": 43.23,
        "veryDeepBooleanKey": true,
        "veryDeepNullKey": null
      }
    }
  }
}
)";
/**
NONE = JSON_TYPE::NONE,
  OBJECT = JSON_TYPE::OBJECT,
  ARRAY = JSON_TYPE::ARRAY,
  NUMBER = JSON_TYPE::NUMBER, // Won't use, not specific enough..
  STRING = JSON_TYPE::STRING,
  BOOL_TYPE = JSON_TYPE::BOOL_TYPE,
  NULL_TYPE = JSON_TYPE::NULL_TYPE,
  INT_TYPE,
  REAL_TYPE
*/

void to_json(std::ostream& oss, json_path::json_node result) {
  switch(result.type) {
      case(json_path::JSON_PATH_TYPE::OBJECT): {
          to_json_object(oss, result.as<std::map<std::string, json_path::json_node>>());
        break;
      }
      case(json_path::JSON_PATH_TYPE::ARRAY): {
          to_json_array(oss, result.as<std::vector<json_path::json_node>>());
        break;
      }
      case(json_path::JSON_PATH_TYPE::STRING): {
        oss << "\"" << result.as<std::string>() << "\"";
        break;
      }
      case(json_path::JSON_PATH_TYPE::BOOL_TYPE): {
        oss << (result.as<bool>() ? "true" : "false");
        break;
      }
      case(json_path::JSON_PATH_TYPE::NULL_TYPE): {
        oss << "null";
        break;
      }
      case(json_path::JSON_PATH_TYPE::INT_TYPE): {
        oss << result.as<json_parce_int_t>();
        break;
      }
      case(json_path::JSON_PATH_TYPE::REAL_TYPE): {
        oss << result.as<json_parce_real_t>();
        break;
      }
      default: {
        oss << "\n\nINVALID TYPE: " << result.type << "\n\n";
      }
    }
}

void to_json_array(std::ostream& oss, std::vector<json_path::json_node> results) {
  using json_path::json_node;
  oss << "[ ";
  bool first = true;
  for (json_path::json_node& result : results) {
    if (!first) {
      oss << ", ";
    } else {
      first = false;
    }
    to_json(oss, result);
  }
  oss << " ]";
}

void to_json_object(std::ostream& oss, std::map<std::string, json_path::json_node> results) {
  oss << "{ ";
  bool first = true;
  for (auto& pair : results) {
    if (!first) {
      oss << ", ";
    } else {
      first = false;
    }
    oss << "\"" << pair.first << "\": ";
    to_json(oss, pair.second);
  }
  oss << " }";
}

int main() {

  json_path::json_path path(json);

  json_path::json_node new_path = path["objectKey"]["innerArrayKey"][0]["deepInnerStringKey"];

  // std::cout << "Result: " << new_path.as<std::string>() << std::endl;

  // std::cout << "multiple_children\n";
  json_path::json_path multiple_children = path["objectKey"]["innerObjectKey"]["deepObjectArrayKey"](0,1,2);
  std::vector<json_path::json_node> results = multiple_children.get<std::vector<json_path::json_node>>();

  // to_json_array(std::cout, results);
  // std::cout << "\n";

  // std::cout << "all_children\n";
  json_path::json_path all_children = path["objectKey"]["innerObjectKey"]["deepObjectArrayKey"]('*');
  results = all_children.get<std::vector<json_path::json_node>>();
  // to_json_array(std::cout, results);
  // std::cout << "\n";

  // std::cout << "every_immediate_children\n";
  json_path::json_path every_immediate_children = path('*');
  results = every_immediate_children.get<std::vector<json_path::json_node>>();
  // to_json_array(std::cout, results);
  // std::cout << "\n";

  std::string rfc_example = R"(
    {
      "o": {"j": 1, "k": 2},
      "a": [5, 3, [{"j": 4}, {"k": 6}]]
    }
  )";
  json_path::json_path descendant_path(rfc_example);

  // std::cout << "every_descendant_value\n";
  json_path::json_path every_descendant_value = descendant_path.descendant().wildcard();
  results = every_descendant_value.get<std::vector<json_path::json_node>>();
  // to_json_array(std::cout, results);
  // std::cout << "\n";

  // std::cout << "get the value of key j from every descendent\n";
  results = descendant_path.descendant()["j"].get<std::vector<json_path::json_node>>();
  // to_json_array(std::cout, results);
  // std::cout << "\n";


  // std::cout << "slice!\n";
  rfc_example = R"(["a", "b", "c", "d", "e", "f", "g"])";
  int start = 1;
  int end = 3;
  results = json_path::json_path(rfc_example).slice(&start, &end).get<std::vector<json_path::json_node>>();
  // to_json_array(std::cout, results);
  // std::cout << "\n";

  start = 5;
  results = json_path::json_path(rfc_example).slice(&start).get<std::vector<json_path::json_node>>();
  // to_json_array(std::cout, results);
  // std::cout << "\n";

  start = 1;
  end = 5;
  int step = 2;
  results = json_path::json_path(rfc_example).slice(&start, &end, &step).get<std::vector<json_path::json_node>>();
  // to_json_array(std::cout, results);
  // std::cout << "\n";

  start = 5;
  end = 1;
  step = -2;
  results = json_path::json_path(rfc_example).slice(&start, &end, &step).get<std::vector<json_path::json_node>>();
  // to_json_array(std::cout, results);
  // std::cout << "\n";

  step = -1;
  results = json_path::json_path(rfc_example).slice(nullptr, nullptr, &step).get<std::vector<json_path::json_node>>();
  // to_json_array(std::cout, results);
  // std::cout << "\n";

  // test_filter_expressions() ;
  rfc_test_filter_selector();

  return 0;
}

void test_filter_expressions() {
  std::string rfc_example = R"(
{
  "a": [3, 5, 1, 2, 4, 6,
        {"b": "j"},
        {"b": "k"},
        {"b": {}},
        {"b": "kilo"}
       ],
  "o": {"p": 1, "q": 2, "r": 3, "s": 5, "t": {"u": 6}},
  "e": "f"
}
  )";

  std::vector<std::string> a_expressions = {
    "?@.b == 'kilo'",
    "?(@.b == 'kilo')",
    "?(@.b == 'kilo' || @.b > 'a' && !@.v)", // 1. (), 2. !, 3. ==, 4. >, 5. &&, 6. ||
    "?@>3.5",
    "?@.b",
    "?@<2 || @.b == \"k\"",
    "?match(@.b, \"[jk]\")",
    "?search(@.b, \"[jk]\")",
    "?@.b == $.x",
    "?@ == @"
  };

  std::vector<std::string> root_expressions = {
    "?@.*",
    "?@[?@.b]",
  };

  std::vector<std::string> o_expressions = {
    "?@<3, ?@<3",
    "?@>1 && @<4",
    "?@.u || @.x",
  };

  json_path::json_path root(rfc_example);
  json_path::json_node root_a = root["a"];
  json_path::json_node root_o = root["o"];

  for (auto& expr : a_expressions) {
    std::cout << expr << ":\n";
    json_path::filter_expression expression(
      std::make_shared<json_path::json_node>(root), 
      std::make_shared<json_path::json_node>(root_a), 
      expr
    );
    json_path::json_node_t returned_value = expression.parse();
    if (returned_value->type == ARRAY) {
      to_json_array(std::cout, returned_value->as<std::vector<json_path::json_node>>());
    } else {
      std:: cout << "Expected array back, got type: " << returned_value->type;
    }
    std::cout << "\n";
  }

  for (auto& expr : root_expressions) {
    std::cout << expr << ":\n";
    json_path::filter_expression expression(
      std::make_shared<json_path::json_node>(root), 
      std::make_shared<json_path::json_node>(root), 
      expr
    );
    json_path::json_node_t returned_value = expression.parse();
    if (returned_value->type == ARRAY) {
      to_json_array(std::cout, returned_value->as<std::vector<json_path::json_node>>());
    } else {
      std:: cout << "Expected array back, got type: " << returned_value->type;
    }
    std::cout << "\n";
  }

  for (auto& expr : o_expressions) {
    std::cout << expr << ":\n";
    json_path::filter_expression expression(
      std::make_shared<json_path::json_node>(root), 
      std::make_shared<json_path::json_node>(root_o), 
      expr
    );
    json_path::json_node_t returned_value = expression.parse();
    if (returned_value->type == ARRAY) {
      to_json_array(std::cout, returned_value->as<std::vector<json_path::json_node>>());
    } else {
      std:: cout << "Expected array back, got type: " << returned_value->type;
    }
    std::cout << "\n";
  }
}


#define TEST_FILTER(NODE, FILTER) \
std::cout << #NODE << "[" << FILTER << "]:\n"; \
result = NODE.filter(FILTER); \
to_json_array(std::cout, result->as<std::vector<json_path::json_node>>()); \
std::cout << "\n"

void rfc_test_filter_selector() {
  std::string str = R"({
    "a": [
      3,
      5,
      1,
      2,
      4,
      6,
      {
        "b": "j"
      },
      {
        "b": "k"
      },
      {
        "b": {}
      },
      {
        "b": "kilo"
      }
    ],
    "o": {
      "p": 1,
      "q": 2,
      "r": 3,
      "s": 5,
      "t": {
        "u": 6
      }
    },
    "e": "f"
  })";

  json_path::json_path root(str);
  json_path::json_node_t result;

  TEST_FILTER(root["a"], "?@.b == 'kilo'");
  TEST_FILTER(root["a"], "?(@.b == 'kilo')");
  TEST_FILTER(root["a"], "?@>3.5");
  TEST_FILTER(root["a"], "?@.b");
  TEST_FILTER(root, "?@.*");
  TEST_FILTER(root, "?@[?@.b]");
  TEST_FILTER(root["o"], "?@<3, ?@<3");
  TEST_FILTER(root["a"], "?@<2 || @.b == \"k\"");
  TEST_FILTER(root["a"], "?match(@.b, \"[jk]\")");
  TEST_FILTER(root["a"], "?search(@.b, \"[jk]\")");
  TEST_FILTER(root["o"], "?@>1 && @<4");
  TEST_FILTER(root["o"], "?@>1 && @<4");
  TEST_FILTER(root["o"], "?@.u || @.x");
  TEST_FILTER(root["a"], "?@.b == $.x");
  TEST_FILTER(root["a"], "?@ == @");
}