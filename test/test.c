#include "c_json_parser.h"
#include "encoding.h"
#include <assert.h>
#include <locale.h>
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#endif

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
#ifdef _WIN32
  // setlocale(LC_CTYPE, ".utf8");
  SetConsoleOutputCP(65001); // Set windows output to utf-8
#endif
  printf("[%d]:<%.*s>\n", index, value_length, value);
  return 0;
}

int on_object_key_value_pair_cb(json_parser *parser, const char *key,
                                size_t key_length, const char *value,
                                size_t value_length) {
#ifdef _WIN32
  // setlocale(LC_CTYPE, ".utf8");
  SetConsoleOutputCP(65001); // Set windows output to utf-8
#endif
  printf("\"%.*s\": <%.*s>\n", key_length, key, value_length, value);
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

static void print_retval(size_t retval, size_t expected, json_parser *parser) {
  static int calls = 0;
  calls++;
  printf("%d) retval: %lld, expected: %lld\n", calls, retval, expected);
  if ((retval != expected) || (parser->err != 0)) {
    printf("%d) %s (%d) \n", calls, json_errno_messages[parser->err],
           parser->err);
  }

  if (parser->state != 37) {
    printf("%d) state is not done (37) state: %d \n", calls, parser->state);
  }

  if (parser->line != 0) {
#ifdef __func__
    printf("%d) %s.%s(%d)\n\n", calls, parser->func, parser->file,
           parser->line);
#else
    printf("%d) %s(%d)\n\n", calls, parser->file, parser->line);
#endif
  }

  assert(retval == expected);
  assert(parser->err == 0);
  assert(parser->state == 37);
}

int main() {
  json_parser parser;

  json_parser_callbacks_typed tcbs = {.on_array_value = on_typed_array_value_cb,
                                      .on_object_key_value_pair =
                                          on_typed_object_value_cb};

  json_parser_callbacks cbs = {.on_array_value = on_array_value_cb,
                               .on_object_key_value_pair =
                                   on_object_key_value_pair_cb};

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

  // 1
  json_parser_init(&parser);
  retval = json_parser_execute_file(&parser, &cbs, "./encodings/utf-8.json");
  print_retval(retval, 48, &parser);

  // 2
  json_parser_init(&parser);
  retval = json_parser_execute_file(&parser, &cbs, "./encodings/utf-16LE.json");
  print_retval(retval, 48, &parser);

  // 3
  json_parser_init(&parser);
  retval =
      json_parser_execute_file(&parser, &cbs, "./encodings/utf-16LEBOM.json");
  print_retval(retval, 51, &parser);

  // 4
  json_parser_init(&parser);
  retval =
      json_parser_execute_file(&parser, &cbs, "./encodings/utf-16BOM2.json");
  print_retval(retval, 51, &parser);

  // 5
  json_parser_init(&parser);
  retval = json_parser_execute_file(&parser, &cbs, "./encodings/utf-32LE.json");
  print_retval(retval, 48, &parser);

  // 6
  json_parser_init(&parser);
  retval =
      json_parser_execute_file(&parser, &cbs, "./encodings/utf-32LEBOM.json");
  print_retval(retval, 51, &parser);
}