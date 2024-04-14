#include "parser.h"
#include <assert.h>
#include <stdio.h>

static const char array_data[] = " \
[ \
  [], \
  null, \
  true, \
  false, \
  \"string\", \
  { \
    \"array\": [], \
    \"null\": null, \
    \"string\": \"[]\", \
    \"unicode\": \"\u2022\", \
    \"escapedString\": \"This is a backslash: \\\\\", \
    \"specialCharacters\": \"This string contains special characters: \b\f\n\r\t\v\" \
  }, \
  \"This string has a \\u0011 control character\", \
  \"\\uD800\\uDF00\" \
] \
";
static const size_t array_data_len = sizeof(array_data) - 1;

static const char object_data[] = "   \
{\
  \"s_parse_objectnullValue\": null,\
  \"emptyString\": \"\",\
  \"escapedString\": \"This is a backslash: \\\\\",\
  \"multilineString\": \"Line 1\nLine 2\nLine 3\",\
  \"unicodeString\": \"\u2022 Unicode Bullet •\",\
  \"numberAsString\": \"42\",\
  \"booleanAsString\": \"true\",\
  \"arrayWithNullAndEmpty\": [null, \"\", \"value\",[[[[[[\"[]]]]]]]]]\"]]]]],[],[],[],[]]],\
  \"objectWithSpecialKeys\": {\
    \"\": \"Empty Key\",\
    \"special-key\": \"Special Key with Hyphen\",\
    \"123\": \"Numeric Key\"\
  },\
  \"deeplyNestedObject\": {\
    \"level1\": {\
      \"level2\":\
      {\
        \"level3\": {\
          \"level4\": {\
            \"value\": \"Deeply Nested Value\"\
          }\
        }\
      }\
    }\
  },\
  \"longString\": \"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.\"\
  \"integer\": 42, \
  \"floatingPoint\": 3.14,\
  \"scientificNotation\": 1.5e10,\
  \"negativeInteger\": -123,\
  \"negativeFloatingPoint\": -0.001, \
  \"\": \"\" \
}\
";
static const size_t object_data_len = sizeof(object_data) - 1;

static int on_array(json_parser *parser, unsigned int index, const char *value,
                    size_t value_length) {
  (void)parser;
  printf("index: %d - value: %.*s\n", index, value_length, value);
  return 0;
}

static int on_key_value_pair(json_parser *parser, const char *key,
                             unsigned int key_length, const char *value,
                             unsigned int value_length) {
  (void)parser;
  printf("key: <%.*s> - value: <%.*s>\n", key_length, key, value_length, value);
  return 0;
}

static json_parser_callbacks cbs = {
    .on_object_key_value_pair = on_key_value_pair, .on_array_value = on_array};

size_t array_test();
size_t object_test();

int main() {
  size_t retval = 0;
  printf("BEGIN array_test\n");
  retval = array_test();
  printf("END array_test <%zd>\n", retval);

  printf("BEGIN object_test\n");
  retval = object_test();
  printf("END object_test <%zd>\n", retval);
  return 0;
}

size_t array_test() {
  json_parser parser;
  json_parser_init(&parser);
  size_t retval =
      json_parser_execute(&parser, &cbs, array_data, array_data_len);
  assert(retval == array_data_len);
  return retval;
}

size_t object_test() {
  json_parser parser;
  json_parser_init(&parser);
  size_t retval =
      json_parser_execute(&parser, &cbs, object_data, object_data_len);
  assert(retval == object_data_len);
  return retval;
}
