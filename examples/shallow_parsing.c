#include "c_json_parser.h"
#include <assert.h>
#include <stdio.h>

static int on_typed_object_value_cb(json_parser *parser, const char *key,
                                    size_t key_length, JSON_TYPE type,
                                    const char *value, size_t value_length) {
  (void)parser;
  printf("Key: \"%.*s\" Value type: %d Value: <%.*s>\n", (int)key_length, key,
         type, (int)value_length, value);
  return 0;
}

static int on_typed_array_value_cb(json_parser *parser, unsigned int index,
                                   JSON_TYPE type, const char *value,
                                   size_t value_length) {
  (void)parser;
  printf("Index: [%d] Value type: %d Value: <%.*s>\n", index, type,
         (int)value_length, value);
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

  json_parser_init(&parser);
  size_t retval =
      json_parser_typed_execute(&parser, &tcbs, array_data, array_data_len);
  assert(retval == array_data_len);

  printf("\n");

  json_parser_init(&parser);
  retval =
      json_parser_typed_execute(&parser, &tcbs, object_data, object_data_len);
  assert(retval == object_data_len);

  return 0;
}