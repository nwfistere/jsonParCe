#include "encoding.h"
#include "json_parce.h"
#include "string.h"
#include <assert.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif

#define QUOTE(X) #X
#define STR(X) QUOTE(X)

#define TEST_ASSERT(EXPR)                                                      \
  if (!(EXPR)) {                                                               \
    fflush(stderr);                                                            \
    fflush(stdout);                                                            \
    fprintf(stderr, "%s(%d): ASSERTION FAILED: \"%s\"\n", __FILE__, __LINE__,  \
            #EXPR);                                                            \
    fflush(stderr);                                                            \
    exit(1);                                                                   \
  }

#define TEST_ASSERT_EQUALS(EXPECTED, ACTUAL)                                   \
  if ((EXPECTED) != (ACTUAL)) {                                                \
    fflush(stderr);                                                            \
    fflush(stdout);                                                            \
    fprintf(stderr,                                                            \
            "%s(%d): ASSERTION EQUALITY FAILED: \"%s == %s\" \"" STR(          \
                EXPECTED) " != " STR(ACTUAL) "\"\n",                           \
            __FILE__, __LINE__, #EXPECTED, #ACTUAL);                           \
    fflush(stderr);                                                            \
    exit(1);                                                                   \
  }

static char *test_testname_string = NULL;

static const char array_data[] =
    "[null,true,false,\"str\\\\ing\",{\"\":\"\\\\\"},[\"str\\\\\"],-1.23e15]";
static const size_t array_data_len = sizeof(array_data) - 1;

static const char object_data[] =
    "{\"\\\\\":\"\", \"null\": null, \"true\": true, \"false\": false, "
    "\"integer\": 1234,\n\t"
    "\"number\":-10.35E1234   , \"string\": \"str\\\\ing\", "
    "\"object\":{\"\\\\\":\"\\\\\"}, \"array\":[\"\\\\\"]}";
static const size_t object_data_len = sizeof(object_data) - 1;

static int print_values(char *filename, int depth, char *parent, char *key,
                        int index, int type, char *value);
static int print_header();
static void get_log_string(const size_t maxlen, const char *str, size_t len,
                           char **retstr, size_t *retlen);
static void print_retval(size_t retval, size_t expected, json_parce *parser);
static void validate_value(JSON_TYPE type, const char *value,
                           size_t value_length);

int on_object_value_cb(json_parce *parser, const char *key, size_t key_length,
                       JSON_TYPE type, const char *value, size_t value_length) {
#ifdef JSON_PARCE_ENABLE_TEST_DEBUG
  char *log_value = NULL;
  size_t log_value_len = 0;
  char log_key[100] = {0};

  get_log_string(20, value, value_length, &log_value, &log_value_len);
  snprintf(log_key, key_length + 1, "%s", key);

  print_values(test_testname_string, 0, "{ROOT}", log_key, -1, type, log_value);
  free(log_value);
#endif
  validate_value(type, value, value_length);
  return 0;
}

int on_array_value_cb(json_parce *parser, size_t index, JSON_TYPE type,
                      const char *value, size_t value_length) {
#ifdef JSON_PARCE_ENABLE_TEST_DEBUG
  char *log_value = NULL;
  size_t log_value_len = 0;
  get_log_string(20, value, value_length, &log_value, &log_value_len);

  print_values(test_testname_string, 0, "[ROOT]", NULL, index, type, log_value);
  free(log_value);
#endif
  validate_value(type, value, value_length);
  return 0;
}

int on_deep_array_value_cb(json_parce *parser, size_t index, JSON_TYPE type,
                           const char *value, size_t value_length) {
#ifdef JSON_PARCE_ENABLE_TEST_DEBUG
  char *log_value = NULL;
  size_t log_value_len = 0;
  get_log_string(20, value, value_length, &log_value, &log_value_len);

  char parent[100] = {0};
  if (parser->current_depth->key) {
    sprintf(parent, "%.*s", (int)parser->current_depth->key_len,
            parser->current_depth->key);
  } else if (parser->current_depth->depth > 0) {
    sprintf(parent, "%d", (int)parser->current_depth->array_index);
  } else {
    sprintf(parent, "%s", "[ROOT]");
  }

  print_values(test_testname_string, parser->current_depth->depth, parent, NULL,
               -1, type, log_value);
  free(log_value);
#endif
  validate_value(type, value, value_length);
  return 0;
}

int on_deep_object_key_value_pair_cb(json_parce *parser, const char *key,
                                     size_t key_length, JSON_TYPE type,
                                     const char *value, size_t value_length) {
#ifdef JSON_PARCE_ENABLE_TEST_DEBUG
  char *log_value = NULL;
  size_t log_value_len = 0;
  get_log_string(20, value, value_length, &log_value, &log_value_len);

  char parent[100] = {0};
  char log_key[100] = {0};
  if (parser->current_depth->key) {
    sprintf(parent, "%.*s", (int)parser->current_depth->key_len,
            parser->current_depth->key);
  } else if (parser->current_depth->depth > 0) {
    sprintf(parent, "%d", (int)parser->current_depth->array_index);
  } else {
    sprintf(parent, "%s", "{ROOT}");
  }

  snprintf(log_key, key_length + 1, "%s", key);

  print_values(test_testname_string, parser->current_depth->depth, parent,
               log_key, -1, type, log_value);
  free(log_value);
#endif
  validate_value(type, value, value_length);
  return 0;
}

int on_start(json_parce *parser, JSON_TYPE type) {
#ifdef JSON_PARCE_ENABLE_TEST_DEBUG
  print_values(
      test_testname_string,
      ((parser->current_depth != NULL) ? parser->current_depth->depth : 0),
      NULL, NULL, -1, type, "START");
#endif
  return 0;
}

int on_end(json_parce *parser, JSON_TYPE type) {
#ifdef JSON_PARCE_ENABLE_TEST_DEBUG
  print_values(
      test_testname_string,
      ((parser->current_depth != NULL) ? parser->current_depth->depth : 0),
      NULL, NULL, -1, type, "END");
  printf("\n");
#endif
  return 0;
}

int test_parsing();
int test_json_parce_string();
int test_process_unicode_string();
int test_mbstrtoc16();
int test_mbstrtoc32();

int main(int argc, char **argv) {

  if (argc != 2) {
    fprintf(stderr, "Usage:\n %s <test_file_dir>\n", argv[0]);
    exit(1);
  }

#ifdef _WIN32
  if (!setlocale(LC_ALL, ".utf8")) {
    fprintf(stderr, "setlocale failed!\n");
  }
#else
  if (!setlocale(LC_ALL, "C.utf8")) {
    fprintf(stderr, "setlocale failed!\n");
  }
#endif

  json_parce parser;

  json_parce_callbacks cbs = {.on_array_value = on_array_value_cb,
                              .on_object_key_value_pair = on_object_value_cb,
                              .on_start = on_start,
                              .on_end = on_end};

  json_parce_callbacks deep_cbs = {.on_array_value = on_deep_array_value_cb,
                                   .on_object_key_value_pair =
                                       on_deep_object_key_value_pair_cb,
                                   .on_start = on_start,
                                   .on_end = on_end};

  print_header();

  // Test array
  test_testname_string = "test array";
  json_parce_init(&parser);
  size_t retval = json_parce_execute(&parser, &cbs, array_data, array_data_len);
  print_retval(retval, array_data_len, &parser);

  // Test object
  test_testname_string = "test object";
  json_parce_init(&parser);
  retval = json_parce_execute(&parser, &cbs, object_data, object_data_len);
  print_retval(retval, object_data_len, &parser);

  // Test deep array
  json_parce_init(&parser);
  test_testname_string = "test deep array";
  retval =
      json_deep_parce_execute(&parser, &deep_cbs, array_data, array_data_len);
  print_retval(retval, array_data_len, &parser);
  json_parce_free(&parser);

  // Test deep object
  json_parce_init(&parser);
  test_testname_string = "test deep object";
  retval =
      json_deep_parce_execute(&parser, &deep_cbs, object_data, object_data_len);
  print_retval(retval, object_data_len, &parser);
  json_parce_free(&parser);

  char test_file[2048] = {0};

  // 1
  json_parce_init(&parser);
  test_testname_string = "utf-8.json";
  snprintf(test_file, sizeof(test_file), "%s/%s", argv[1],
           test_testname_string);
  retval = json_parce_execute_file(&parser, &cbs, test_file);
  print_retval(retval, 48, &parser);

  // 2
  json_parce_init(&parser);
  test_testname_string = "utf-8BOM.json";
  snprintf(test_file, sizeof(test_file), "%s/%s", argv[1],
           test_testname_string);
  retval = json_parce_execute_file(&parser, &cbs, test_file);
  print_retval(retval, 51, &parser);

  // 3
  json_parce_init(&parser);
  test_testname_string = "utf-16LE.json";
  snprintf(test_file, sizeof(test_file), "%s/%s", argv[1],
           test_testname_string);
  retval = json_parce_execute_file(&parser, &cbs, test_file);
  print_retval(retval, 48, &parser);

  // 4
  json_parce_init(&parser);
  test_testname_string = "utf-16BE.json";
  snprintf(test_file, sizeof(test_file), "%s/%s", argv[1],
           test_testname_string);
  retval = json_parce_execute_file(&parser, &cbs, test_file);
  print_retval(retval, 48, &parser);

  // 5
  json_parce_init(&parser);
  test_testname_string = "utf-16LEBOM.json";
  snprintf(test_file, sizeof(test_file), "%s/%s", argv[1],
           test_testname_string);
  retval = json_parce_execute_file(&parser, &cbs, test_file);
  print_retval(retval, 51, &parser);

  // 6
  json_parce_init(&parser);
  test_testname_string = "utf-16BEBOM.json";
  snprintf(test_file, sizeof(test_file), "%s/%s", argv[1],
           test_testname_string);
  retval = json_parce_execute_file(&parser, &cbs, test_file);
  print_retval(retval, 51, &parser);

  // 7
  json_parce_init(&parser);
  test_testname_string = "utf-32LE.json";
  snprintf(test_file, sizeof(test_file), "%s/%s", argv[1],
           test_testname_string);
  retval = json_parce_execute_file(&parser, &cbs, test_file);
  print_retval(retval, 48, &parser);

  // 8
  json_parce_init(&parser);
  test_testname_string = "utf-32BE.json";
  snprintf(test_file, sizeof(test_file), "%s/%s", argv[1],
           test_testname_string);
  retval = json_parce_execute_file(&parser, &cbs, test_file);
  print_retval(retval, 48, &parser);

  // 9
  json_parce_init(&parser);
  test_testname_string = "utf-32LEBOM.json";
  snprintf(test_file, sizeof(test_file), "%s/%s", argv[1],
           test_testname_string);
  retval = json_parce_execute_file(&parser, &cbs, test_file);
  print_retval(retval, 51, &parser);

  // 10
  json_parce_init(&parser);
  test_testname_string = "utf-32BEBOM.json";
  snprintf(test_file, sizeof(test_file), "%s/%s", argv[1],
           test_testname_string);
  retval = json_parce_execute_file(&parser, &cbs, test_file);
  print_retval(retval, 51, &parser);

  test_parsing();
  test_json_parce_string();
  test_process_unicode_string();
  test_mbstrtoc16();
  test_mbstrtoc32();
}

static int test_parsing_on_array_value_cb(json_parce *parser, size_t index,
                                          JSON_TYPE type, const char *value,
                                          size_t value_length) {
  switch (index) {
  case 0: {
    TEST_ASSERT_EQUALS(type, NULL_TYPE);
    TEST_ASSERT(strncmp(value, "null", value_length) == 0);
    break;
  }
  case 1: {
    TEST_ASSERT_EQUALS(type, BOOL_TYPE);
    TEST_ASSERT(strncmp(value, "true", value_length) == 0);
    break;
  }
  case 2: {
    TEST_ASSERT_EQUALS(type, BOOL_TYPE);
    TEST_ASSERT(strncmp(value, "false", value_length) == 0);
    break;
  }
  case 3: {
    TEST_ASSERT_EQUALS(type, STRING);
    TEST_ASSERT(strncmp(value, "str\\\\ing", value_length) == 0);
    break;
  }
  case 4: {
    TEST_ASSERT_EQUALS(type, OBJECT);
    break;
  }
  case 5: {
    TEST_ASSERT_EQUALS(type, ARRAY);
    break;
  }
  case 6: {
    TEST_ASSERT_EQUALS(type, NUMBER);
    TEST_ASSERT(strncmp(value, "-1.23e15", value_length) == 0);
    break;
  }
  default: {
    fprintf(stderr, "Not a known index \"%d\"", (int)index);
    TEST_ASSERT(0);
  }
  }

  return 0;
}

static int test_parsing_on_object_value_cb(json_parce *parser, const char *key,
                                           size_t key_length, JSON_TYPE type,
                                           const char *value,
                                           size_t value_length) {
  if (strncmp(key, "null", key_length) == 0) {
    TEST_ASSERT_EQUALS(type, NULL_TYPE);
  } else if (strncmp(key, "true", key_length) == 0) {
    TEST_ASSERT_EQUALS(type, BOOL_TYPE);
  } else if (strncmp(key, "false", key_length) == 0) {
    TEST_ASSERT_EQUALS(type, BOOL_TYPE);
  } else if (strncmp(key, "integer", key_length) == 0) {
    TEST_ASSERT_EQUALS(type, NUMBER);
  } else if (strncmp(key, "number", key_length) == 0) {
    TEST_ASSERT_EQUALS(type, NUMBER);
  } else if (strncmp(key, "string", key_length) == 0) {
    TEST_ASSERT_EQUALS(type, STRING);
  } else if (strncmp(key, "object", key_length) == 0) {
    TEST_ASSERT_EQUALS(type, OBJECT);
  } else if (strncmp(key, "array", key_length) == 0) {
    TEST_ASSERT_EQUALS(type, ARRAY);
  } else if (strncmp(key, "\\\\", key_length) == 0) {
    TEST_ASSERT_EQUALS(value_length, 0);
  } else {
    fprintf(stderr, "Not a known key \"%.*s\"", (int)key_length, key);
    TEST_ASSERT(0);
  }

  return 0;
}

int test_parsing() {
  json_parce_callbacks cbs = {.on_array_value = test_parsing_on_array_value_cb,
                              .on_object_key_value_pair =
                                  test_parsing_on_object_value_cb};

  json_parce parser;
  json_parce_init(&parser);

  size_t retval =
      json_parce_execute(&parser, &cbs, object_data, object_data_len);

  TEST_ASSERT_EQUALS(retval, object_data_len);
  TEST_ASSERT_EQUALS(parser.err, 0);

  print_retval(retval, object_data_len, &parser);

  return 0;
}

int print_values(char *filename, int depth, char *parent, char *key, int index,
                 int type, char *value) {
#ifdef JSON_PARCE_ENABLE_TEST_DEBUG
#ifdef _WIN32
  SetConsoleOutputCP(65001); // Set windows output to utf-8
#endif
  printf("%65s", filename ? filename : "-");
  printf("%10d", depth);
  printf("%10s", parent ? parent : "-");
  printf("%10s", key ? key : "-");
  if (index > -1) {
    printf("%10d", index);
  } else {
    printf("%10s", "-");
  }
  printf("%10d", type);
  printf("%15s\n", value);
#endif
  return 0;
}

int print_header() {
#ifdef JSON_PARCE_ENABLE_TEST_DEBUG
  printf("%65s%10s%10s%10s%10s%10s%15s\n", "Filename", "Depth", "Parent", "Key",
         "Index", "Type", "Value");
#endif
  return 0;
}

void get_log_string(const size_t maxlen, const char *str, size_t len,
                    char **retstr, size_t *retlen) {
  *retstr = (char *)malloc((maxlen + 3 + 1) * sizeof(char));
  memset(*retstr, 0, (maxlen + 3 + 1) * sizeof(char));
  int j = 0;
  for (int i = 0; ((i < len) && (j < maxlen)); i++) {
    switch (str[i]) {
    case '\n':
      (*retstr)[j++] = '\\';
      (*retstr)[j] = 'n';
      break;
    case '\r':
      (*retstr)[j++] = '\\';
      (*retstr)[j] = 'r';
      break;
    case '\t':
      (*retstr)[j++] = '\\';
      (*retstr)[j] = 't';
      break;
    case '\v':
      (*retstr)[j++] = '\\';
      (*retstr)[j] = 'v';
      break;
    case '\f':
      (*retstr)[j++] = '\\';
      (*retstr)[j] = 'f';
      break;
    default:
      (*retstr)[j] = str[i];
    }
    j++;
  }
  if (len > maxlen) {
    for (int i = 0; i < 3; i++) {
      (*retstr)[j++] = '.';
    }
  }
  *retlen = j;
}

void print_retval(size_t retval, size_t expected, json_parce *parser) {
  static int calls = 0;
  calls++;
  if (retval != expected) {
    printf("%d) retval: %zd, expected: %zd\n", calls, retval, expected);
  }
  if ((retval != expected) || (parser->err != 0)) {
    printf("%d) %s (%d) \n", calls, json_errno_messages[parser->err],
           parser->err);
  }

  if (parser->state != JSON_PARCE_DONE_STATE) {
    printf("%d) state is not done (38) state: %d \n", calls, parser->state);
  }

  if (parser->line != 0) {
#ifdef __func__
    printf("%d) %s.%s(%d)\n\n", calls, parser->func, parser->file,
           parser->line);
#else
    printf("%d) %s(%d)\n\n", calls, parser->file, parser->line);
#endif
  }

  TEST_ASSERT_EQUALS(retval, expected);
  TEST_ASSERT_EQUALS(parser->err, 0);
  TEST_ASSERT_EQUALS(parser->state, JSON_PARCE_DONE_STATE);
}

void validate_value(JSON_TYPE type, const char *value, size_t value_length) {
  switch (type) {
  case NUMBER: {
    json_parce_int_t rint;
    json_parce_real_t real;
    int ret = json_parce_int(value, value_length, &rint);
    if (ret != 0) {
      ret = json_parce_real(value, value_length, &real);
    }
    if (ret != 0) {
      fprintf(stderr, "\n\n [%s] - Failed to parse number: <%.*s>\n",
              test_testname_string, (int)value_length, value);
    }
    break;
  }
  case STRING: {
    char *str = json_parce_string(value, value_length);
    if (str != NULL) {
      free(str);
    } else {
      fprintf(stderr, "\n\n [%s] - Failed to parse string: <%.*s>\n",
              test_testname_string, (int)value_length, value);
    }
    break;
  }
  case BOOL_TYPE: {
    int ret = json_parce_bool(value);
    TEST_ASSERT((ret == 0) || (ret == 1));
    break;
  }
  default: {
  }
  }
}

int test_json_parce_string() {
  char *item = "\\\"Hello, world\\\"!";
  char *retval = json_parce_string(item, strlen(item));
  char *expected = "\"Hello, world\"!";
  printf("test_json_parce_string - Expecting <%s> == <%s>\n", retval, expected);
  TEST_ASSERT(strcmp(retval, expected) == 0);
  free(retval);

  item = "\\\"\\\"\\\"";
  retval = json_parce_string(item, strlen(item));
  expected = "\"\"\"";
  printf("test_json_parce_string - Expecting <%s> == <%s>\n", retval, expected);
  TEST_ASSERT(strcmp(retval, expected) == 0);
  free(retval);

  item = "\\\"\\'\\/\\b\\r\\n\\t\\f";
  retval = json_parce_string(item, strlen(item));
  expected = "\"'/\b\r\n\t\f";
  printf("test_json_parce_string - Expecting <%s> == <%s>\n", retval, expected);
  TEST_ASSERT(strcmp(retval, expected) == 0);
  free(retval);

  return 0;
}

#define print_str_hex(OUT, STR)                                                \
  for (size_t i = 0; i < strlen(STR); i++) {                                   \
    fprintf((OUT), "%04X", 0xFFFF & STR[i]);                                   \
    if (i < (strlen(STR) - 1)) {                                               \
      fprintf((OUT), " ");                                                     \
    }                                                                          \
  }

#define TEST_UNICODE(INPUT, EXPECTED)                                          \
  status = process_unicode_escape_string((INPUT), &output);                    \
  TEST_ASSERT_EQUALS(status, 0);                                               \
  printf("test_process_unicode_string - Expecting <%s> == <%s>\n", output,     \
         (EXPECTED));                                                          \
  TEST_ASSERT(strcmp(output, (EXPECTED)) == 0);                                \
  if (strcmp(output, (EXPECTED)) != 0) {                                       \
    fprintf(stderr, "test_process_unicode_string - FAILED <");                 \
    print_str_hex(stderr, output) fprintf(stderr, "> != <");                   \
    print_str_hex(stderr, (EXPECTED))                                          \
        fprintf(stderr, "> line: %d\n", __LINE__);                             \
  }                                                                            \
  free(output)

#define TEST_UNICODE_ERROR(INPUT, EXPECTED)                                    \
  status = process_unicode_escape_string((INPUT), &output);                    \
  printf("test_process_unicode_string - Expecting status <%d> == <%d>\n",      \
         status, (EXPECTED));                                                  \
  TEST_ASSERT_EQUALS(status, EXPECTED);

int test_process_unicode_string() {
  int status;
  char *output = NULL;
  printf("\n\ntest_process_unicode_string\n\n");

  TEST_UNICODE("Hello world!", "Hello world!");
  TEST_UNICODE("\\uFFEE", "\xEF\xBF\xAE");
  TEST_UNICODE("\\u0000", "");
  TEST_UNICODE("\\u0021", "!");
  TEST_UNICODE("\\u00FF\\u0021", "\xC3\xBF!");
  TEST_UNICODE("\\uFFE8\\uFFE8\\uFFE8\\uFFE8\\uFFE8\\uFFE8\\uFFE8",
               "\xEF\xBF\xA8\xEF\xBF\xA8\xEF\xBF\xA8\xEF\xBF\xA8\xEF\xBF\xA8"
               "\xEF\xBF\xA8\xEF\xBF\xA8");
  TEST_UNICODE("\\uD83D\\uDE00", "\xF0\x9F\x98\x80");
  TEST_UNICODE("\\uD888\\uDFAF", "\xF0\xB2\x8E\xAF");
  TEST_UNICODE("\\uDBFF\\uDFFD", "\xF4\x8F\xBF\xBD");
  TEST_UNICODE("\\uDB40\\uDDEF", "\xF3\xA0\x87\xAF");
  TEST_UNICODE("\\uD884\\uDF4A", "\xF0\xB1\x8D\x8A");
  TEST_UNICODE("\\uD87E\\uDDF4", "\xF0\xAF\xA7\xB4");
  TEST_UNICODE("\\uD800\\uDC00", "\xF0\x90\x80\x80");

  TEST_UNICODE("\\uD800\\uDC00\\uFFE8", "\xF0\x90\x80\x80\xEF\xBF\xA8");
  TEST_UNICODE("!!\\uD800\\uDC00\\uFFE8!!", "!!\xF0\x90\x80\x80\xEF\xBF\xA8!!");
  TEST_UNICODE("!!\\uD800\\uDC000\\uFFE8!!", "!!\xF0\x90\x80\x80"
                                             "0\xEF\xBF\xA8!!");

  TEST_UNICODE_ERROR("\\uD800!\\uDC00",
                     -1); // character between high and low surrogates.
  TEST_UNICODE_ERROR("\\uD800", -1);        // lonely high surrogate
  TEST_UNICODE_ERROR("\\uDC00", -1);        // lonely low surrogate
  TEST_UNICODE_ERROR("\\uD800\\uD800", -2); // two high surrogate
  TEST_UNICODE_ERROR("\\uD800\\u0021", -2); // high surrogate + utf-8 character.
  TEST_UNICODE_ERROR("\\u0021\\uD800", -1); // utf-8 character + high surrogate.
  TEST_UNICODE_ERROR("\\u0021\\uDC00", -1); // utf-8 character + low surrogate.

  return 0;
}

#define TEST_MBSTRTOC16(INPUT, EXPECTED)                                       \
  do {                                                                         \
    char16_t *out = NULL;                                                      \
    size_t out_sz = 0;                                                         \
    int status = mbstrtoc16((INPUT), strlen(INPUT) + 1, &out, &out_sz);        \
    TEST_ASSERT_EQUALS(status, 0);                                             \
    printf("test_mbstrtoc16 - Expecting <%zd> == <%zd>\n", out_sz - 1,         \
           (strlen16(EXPECTED)));                                              \
    TEST_ASSERT_EQUALS((out_sz - 1), strlen16(EXPECTED));                      \
    TEST_ASSERT(strcmp16(out, (EXPECTED)) == 0);                               \
    free(out);                                                                 \
  } while (0)

int test_mbstrtoc16() {
  TEST_MBSTRTOC16("\xE1\x88\x96", u"\u1216");
  TEST_MBSTRTOC16("", u"");
  TEST_MBSTRTOC16("Hello World!", u"Hello World!");
  TEST_MBSTRTOC16(u8"Hello, 世界! 🐀", u"Hello, 世界! 🐀");
  TEST_MBSTRTOC16("\xF0\x9F\x90\x80", u"\U0001F400");

  return 0;
}

#define TEST_MBSTRTOC32(INPUT, EXPECTED)                                       \
  do {                                                                         \
    char32_t *out = NULL;                                                      \
    size_t out_sz = 0;                                                         \
    int status = mbstrtoc32((INPUT), strlen(INPUT) + 1, &out, &out_sz);        \
    TEST_ASSERT_EQUALS(status, 0);                                             \
    printf("test_mbstrtoc32 - Expecting <%zd> == <%zd>\n", out_sz - 1,         \
           (strlen32(EXPECTED)));                                              \
    TEST_ASSERT_EQUALS((out_sz - 1), strlen32(EXPECTED));                      \
    TEST_ASSERT(strcmp32(out, (EXPECTED)) == 0);                               \
    free(out);                                                                 \
  } while (0)

int test_mbstrtoc32() {
  TEST_MBSTRTOC32("\xE1\x88\x96", U"\u1216");
  TEST_MBSTRTOC32("", U"");
  TEST_MBSTRTOC32("Hello World!", U"Hello World!");
  TEST_MBSTRTOC32(u8"Hello, 世界! 🐀", U"Hello, 世界! 🐀");
  TEST_MBSTRTOC32("\xF0\x9F\x90\x80", U"\U0001F400");

  return 0;
}