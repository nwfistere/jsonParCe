#include "parser.h"
#include <stdio.h>
#include <string.h>

static int on_array(json_parser *parser, unsigned int index, const char *value, size_t value_length) {
  printf("index: %d - value: %.*s", index, value_length, value);
  return 0;
}

static json_parser_callbacks cbs = {
  .on_object_key_value_pair = NULL,
  .on_array_value = on_array
};

static const char* data = " \
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

int main() {
  json_parser parser;

  json_parser_init(&parser);

  json_parser_execute(&parser, &cbs, data, strlen(data));

  return 0;
}