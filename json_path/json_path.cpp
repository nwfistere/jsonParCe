#include "json_path.hpp"

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

json_path::json_node::json_node(const json_path& path) {
  type = path.get().type;
  value = path.get().value;
};



int main() {
  using json_path::json_node;
  using json_path::json_path;
  // json_path::json_path path;

  // path("hello world!")(1)("hello world2!")(0,1,2,3);
  // path("hello world!")(1)("hello world2!")(0,1,2,3);
  // path(0,1,2,3,4);

  json_path path(json);

  json_node new_path = path["objectKey"]["innerArrayKey"][0]["deepInnerStringKey"];

  return 0;
}