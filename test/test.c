#include "parser.h"
#include <assert.h>

static const char array_data[] =
    "[null,true,false,\"str\\\\ing\",{\"\":\"\\\\\"},[\"str\\\\\"],-1.23e15]";
static const size_t array_data_len = sizeof(array_data) - 1;

static const char object_data[] =
    "{\"\\\\\":\"\", \"null\": null, \"true\": true, \"false\": false, "
    "\"number\":-10.35E1234   , \"string\": \"str\\\\ing\", "
    "\"object\":{\"\\\\\":\"\\\\\"}, \"array\":[\"\\\\\"]}";
static const size_t object_data_len = sizeof(object_data) - 1;

int on_array_value_cb(json_parser *parser, unsigned int index,
                      const char *value, size_t value_length) {
  return 0;
}

int on_object_key_value_pair_cb(json_parser *parser, const char *key,
                                size_t key_length, const char *value,
                                size_t value_length) {
  return 0;
}

int on_typed_object_value_cb(json_parser *parser, const char *key,
                             size_t key_length, JSON_TYPE type,
                             const char *value, size_t value_length) {
  return 0;
}

int on_typed_array_value_cb(json_parser *parser, unsigned int index,
                            JSON_TYPE type, const char *value,
                            size_t value_length) {
  return 0;
}

int main() {
  json_parser parser;

  json_parser_callbacks_typed tcbs = {.on_array_value = on_typed_array_value_cb,
                                      .on_object_key_value_pair =
                                          on_typed_object_value_cb};

  // Test array
  json_parser_init(&parser);
  size_t retval =
      json_parser_typed_execute(&parser, &tcbs, array_data, array_data_len);
  assert(retval == array_data_len);

  // Test object
  json_parser_init(&parser);
  retval =
      json_parser_typed_execute(&parser, &tcbs, object_data, object_data_len);
  assert(retval == object_data_len);
}