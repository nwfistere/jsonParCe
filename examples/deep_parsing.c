#include "c_json_parser.h"
#include <assert.h>
#include <stdio.h>

static int on_typed_object_value_cb(json_parser *parser, const char *key,
                                    size_t key_length, JSON_TYPE type,
                                    const char *value, size_t value_length) {
  printf("%10d", parser->current_depth->depth);
  if (parser->current_depth->key) {
    printf("%10.*s", (int)parser->current_depth->key_len,
           parser->current_depth->key);
  } else if (parser->current_depth->depth > 0) {
    printf("%10d", (int)parser->current_depth->array_index);
  } else {
    printf("%10s", "{ROOT}");
  }
  printf("%10.*s%10s%10.*s%10d\n", (int)key_length, key, "-", (int)value_length,
         value, type);
  return 0;
}

static int on_typed_array_value_cb(json_parser *parser, unsigned int index,
                                   JSON_TYPE type, const char *value,
                                   size_t value_length) {
  printf("%10d", parser->current_depth->depth);
  if (parser->current_depth->key) {
    printf("%10.*s", (int)parser->current_depth->key_len,
           parser->current_depth->key);
  } else if (parser->current_depth->depth > 0) {
    printf("%10d", (int)parser->current_depth->array_index);
  } else {
    printf("%10s", "[ROOT]");
  }
  printf("%10s%10d%10.*s%10d\n", "-", index, (int)value_length, value, type);
  return 0;
}

static const char array_data[] =
    "[\"string\", null, true, false, { \"object\": true }, [\"array\"], 1234]";
static const size_t array_data_len = sizeof(array_data) - 1;

static const char object_data[] =
    "{ \"hello\": \"world!\", \"bool1\": true, \"bool2\": false, \"array\": "
    "[1,2,3,4,5], \"object\": { \"null\": null}, \"null\": null}";
static const size_t object_data_len = sizeof(object_data) - 1;

int main(void) {

  json_parser parser;
  json_parser_callbacks_typed tcbs = {.on_array_value = on_typed_array_value_cb,
                                      .on_object_key_value_pair =
                                          on_typed_object_value_cb};

  printf("%10s%10s%10s%10s%10s%10s\n", "Depth", "Parent", "Key", "Index",
         "Value", "Type");
  json_parser_init(&parser);
  size_t retval = json_deep_parser_typed_execute(&parser, &tcbs, array_data,
                                                 array_data_len);
  assert(retval == array_data_len);

  printf("\n");

  printf("%10s%10s%10s%10s%10s%10s\n", "Depth", "Parent", "Key", "Index",
         "Value", "Type");
  json_parser_init(&parser);
  retval = json_deep_parser_typed_execute(&parser, &tcbs, object_data,
                                          object_data_len);
  assert(retval == object_data_len);

  return 0;
}