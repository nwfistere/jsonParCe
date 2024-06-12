#include "json_parce.h"
#include "encoding.h"
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

int on_array_value_cb(json_parce *parser, unsigned int index, JSON_TYPE type,
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

int on_deep_array_value_cb(json_parce *parser, unsigned int index,
                           JSON_TYPE type, const char *value,
                           size_t value_length) {
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

int test_parsing();
int test_JSONTestSuite();
int test_JSONTestSuite_transform();
int test_json_parce_string();
int test_process_unicode_string();

int main() {
  json_parce parser;

  json_parce_callbacks cbs = {.on_array_value = on_array_value_cb,
                              .on_object_key_value_pair = on_object_value_cb};

  json_parce_callbacks deep_cbs = {.on_array_value = on_deep_array_value_cb,
                                   .on_object_key_value_pair =
                                       on_deep_object_key_value_pair_cb};

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

  // 1
  json_parce_init(&parser);
  test_testname_string = "utf-8.json";
  retval = json_parce_execute_file(&parser, &cbs, "./encodings/utf-8.json");
  print_retval(retval, 48, &parser);

  // 2
  json_parce_init(&parser);
  test_testname_string = "utf-8BOM.json";
  retval = json_parce_execute_file(&parser, &cbs, "./encodings/utf-8BOM.json");
  print_retval(retval, 51, &parser);

  // 3
  json_parce_init(&parser);
  test_testname_string = "utf-16LE.json";
  retval = json_parce_execute_file(&parser, &cbs, "./encodings/utf-16LE.json");
  print_retval(retval, 48, &parser);

  // 4
  json_parce_init(&parser);
  test_testname_string = "utf-16BE.json";
  retval = json_parce_execute_file(&parser, &cbs, "./encodings/utf-16BE.json");
  print_retval(retval, 48, &parser);

  // 5
  json_parce_init(&parser);
  test_testname_string = "utf-16LEBOM.json";
  retval =
      json_parce_execute_file(&parser, &cbs, "./encodings/utf-16LEBOM.json");
  print_retval(retval, 51, &parser);

  // 6
  json_parce_init(&parser);
  test_testname_string = "utf-16BEBOM.json";
  retval =
      json_parce_execute_file(&parser, &cbs, "./encodings/utf-16BEBOM.json");
  print_retval(retval, 51, &parser);

  // 7
  json_parce_init(&parser);
  test_testname_string = "utf-32LE.json";
  retval = json_parce_execute_file(&parser, &cbs, "./encodings/utf-32LE.json");
  print_retval(retval, 48, &parser);

  // 8
  json_parce_init(&parser);
  test_testname_string = "utf-32BE.json";
  retval = json_parce_execute_file(&parser, &cbs, "./encodings/utf-32BE.json");
  print_retval(retval, 48, &parser);

  // 9
  json_parce_init(&parser);
  test_testname_string = "utf-32LEBOM.json";
  retval =
      json_parce_execute_file(&parser, &cbs, "./encodings/utf-32LEBOM.json");
  print_retval(retval, 51, &parser);

  // 10
  json_parce_init(&parser);
  test_testname_string = "utf-32BEBOM.json";
  retval =
      json_parce_execute_file(&parser, &cbs, "./encodings/utf-32BEBOM.json");
  print_retval(retval, 51, &parser);

  retval = test_JSONTestSuite();
  assert(retval == 0);

  retval = test_JSONTestSuite_transform();
  assert(retval == 0);

  test_parsing();

  test_json_parce_string();

  test_process_unicode_string();
}

static int test_parsing_on_array_value_cb(json_parce *parser,
                                          unsigned int index, JSON_TYPE type,
                                          const char *value,
                                          size_t value_length) {
  switch (index) {
  case 0: {
    assert(type == NULL_TYPE);
    assert(strncmp(value, "null", value_length) == 0);
    break;
  }
  case 1: {
    assert(type == BOOL_TYPE);
    assert(strncmp(value, "true", value_length) == 0);
    break;
  }
  case 2: {
    assert(type == BOOL_TYPE);
    assert(strncmp(value, "false", value_length) == 0);
    break;
  }
  case 3: {
    assert(type == STRING);
    assert(strncmp(value, "str\\\\ing", value_length) == 0);
    break;
  }
  case 4: {
    assert(type == OBJECT);
    break;
  }
  case 5: {
    assert(type == ARRAY);
    break;
  }
  case 6: {
    assert(type == NUMBER);
    assert(strncmp(value, "-1.23e15", value_length) == 0);
    break;
  }
  default: {
    fprintf(stderr, "Not a known index \"%d\"", index);
    assert(0);
  }
  }

  return 0;
}

static int test_parsing_on_object_value_cb(json_parce *parser, const char *key,
                                           size_t key_length, JSON_TYPE type,
                                           const char *value,
                                           size_t value_length) {
  if (strncmp(key, "null", key_length) == 0) {
    assert(type == NULL_TYPE);
  } else if (strncmp(key, "true", key_length) == 0) {
    assert(type == BOOL_TYPE);
  } else if (strncmp(key, "false", key_length) == 0) {
    assert(type == BOOL_TYPE);
  } else if (strncmp(key, "integer", key_length) == 0) {
    assert(type == NUMBER);
  } else if (strncmp(key, "number", key_length) == 0) {
    assert(type == NUMBER);
  } else if (strncmp(key, "string", key_length) == 0) {
    assert(type == STRING);
  } else if (strncmp(key, "object", key_length) == 0) {
    assert(type == OBJECT);
  } else if (strncmp(key, "array", key_length) == 0) {
    assert(type == ARRAY);
  } else if (strncmp(key, "\\\\", key_length) == 0) {
    assert(value_length == 0);
  } else {
    fprintf(stderr, "Not a known key \"%.*s\"", (int)key_length, key);
    assert(0);
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

  assert(retval == object_data_len);
  assert(parser.err == 0);

  print_retval(retval, object_data_len, &parser);

  return 0;
}

int test_JSONTestSuite_array(json_parce *parser, unsigned int index,
                             JSON_TYPE type, const char *value,
                             size_t value_length) {
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
               index, type, log_value);
  free(log_value);
#endif
  validate_value(type, value, value_length);
  return 0;
}

int test_JSONTestSuite_object(json_parce *parser, const char *key,
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

int test_JSONTestSuite() {
  int retval = 0;
  json_parce parser;

  size_t filesize;
  char *filename;
  char *path = "./test_parsing";
  char filepath[1024] = {0};

  json_parce_callbacks deep_cbs = {.on_array_value = test_JSONTestSuite_array,
                                   .on_object_key_value_pair =
                                       test_JSONTestSuite_object};

#ifdef _WIN32
  WIN32_FIND_DATA fd;
  HANDLE hFind = NULL;

  sprintf(filepath, "%s\\*.json", path);

  if ((hFind = FindFirstFile(filepath, &fd)) == INVALID_HANDLE_VALUE) {
    fprintf(stderr, "Failed to open directory \"%s\"\n", path);
    return -1;
  }

  do {
    if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      // skip directories
      continue;
    }

    sprintf(filepath, "%s\\%s", path, fd.cFileName);
    filesize = fd.nFileSizeLow;
    filename = fd.cFileName;
#else
  DIR *d = opendir(path);

  if (!d) {
    fprintf(stderr, "Failed to open directory \"%s\"", path);
    return -1;
  }

  struct dirent *entry;

  while ((entry = readdir(d)) != NULL) {
#ifdef _DIRENT_HAVE_D_TYPE
    if (entry->d_type != DT_REG) {
      // skip anything that's not a normal file.
      continue;
    }
#endif
    sprintf(filepath, "%s/%s", path, entry->d_name);

    struct stat sb = {0};
    if (stat(filepath, &sb) != 0) {
      fprintf(stderr, "Failed to get file info using stat. \"%s\"", filepath);
      return -1;
    }

    filesize = sb.st_size;
    filename = entry->d_name;
#endif // WIN32

    test_testname_string = filename;

    json_parce_init(&parser);
    size_t retval = json_deep_parce_execute_file(&parser, &deep_cbs, filepath);

    // y_* files should work
    // i_* files may work.
    // n_* files should cause failures.
    if (strncmp(filename, "y_", 2) == 0) {
      static const char *ignore_files[] = {
          "y_string_space.json",
          "y_structure_lonely_false.json",
          "y_structure_lonely_int.json",
          "y_structure_lonely_negative_real.json",
          "y_structure_lonely_null.json",
          "y_structure_lonely_string.json",
          "y_structure_lonely_true.json",
          "y_structure_string_empty.json"};

      int ignore = 0;
      for (int i = 0; i < (sizeof(ignore_files) / sizeof(*ignore_files)); ++i) {
        if (strcmp(ignore_files[i], filename) == 0) {
          ignore = 1;
          break;
        }
      }

      if (!ignore && (parser.err || (filesize != retval))) {
        fprintf(stderr, "\nERROR: \"%s\" FAILED\n", filepath);
        if (filesize != retval) {
          fprintf(stderr, "\texpected: %zd, actual: %zd\n", filesize, retval);
        }
        if (parser.err) {
          fprintf(stderr, "\t%s (%d) \n", json_errno_messages[parser.err],
                  parser.err);
          fprintf(stderr, "\t%s(%d)\n\n", parser.file, parser.line);
        }
        retval++;
      } else if (ignore && !parser.err) {
        printf("\nINFO: \"%s\" COMPLETED successfully but ignored\n", filepath);
      }

    } else if (strncmp(filename, "i_", 2) == 0) {
      // Exclude the utf16 tests from the size check.
      if (parser.err ||
          (filesize != retval && (strstr(filename, "16") == NULL))) {
        fprintf(stderr, "\nWARNING: \"%s\" FAILED\n", filepath);
        if (filesize != retval) {
          fprintf(stderr, "\texpected: %zd, actual: %zd\n", filesize, retval);
        }
        if (parser.err) {
          fprintf(stderr, "\t%s (%d) \n", json_errno_messages[parser.err],
                  parser.err);
          fprintf(stderr, "\t%s(%d) - %s()\n\n", parser.file,
                  parser.line, parser.func);
        }
      }
    } else if (strncmp(filename, "n_", 2) == 0) {
      if (!parser.err) {
#ifdef JSON_PARCE_STRICT_MODE
        // fprintf(stderr, "\"%s\",\n", filename);
        fprintf(stderr, "\nERROR: \"%s\" SUCCESS\n", filepath);
        retval++;
#else
        printf("\nINFO: \"%s\" SUCCESS - (JSON_PARCE_STRICT_MODE is disabled)",
               filepath);
#endif
      }
    } else {
      fprintf(stderr, "\nWARNING: Unexpected file parsed. %s", filepath);
    }
    // free parser in case of failure.
    json_parce_free(&parser);
#ifdef _WIN32
  } while (FindNextFile(hFind, &fd));
  FindClose(hFind);
#else
  } // while loop
  closedir(d);
#endif // WIN32

  return retval;
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

  assert(retval == expected);
  assert(parser->err == 0);
  assert(parser->state == JSON_PARCE_DONE_STATE);
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
      fprintf(stderr, "\n\n [%s] - Failed to parse string: <%.*s>\n", test_testname_string, (int)value_length, value);
    }
    break;
  }
  case BOOL_TYPE: {
    int ret = json_parce_bool(value);
    assert((ret == 0) || (ret == 1));
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
  assert(strcmp(retval, expected) == 0);
  free(retval);

  item = "\\\"\\\"\\\"";
  retval = json_parce_string(item, strlen(item));
  expected = "\"\"\"";
  printf("test_json_parce_string - Expecting <%s> == <%s>\n", retval, expected);
  assert(strcmp(retval, expected) == 0);
  free(retval);

  return 0;
}

int test_JSONTestSuite_transform() {
  printf("\n\ntest_JSONTestSuite_transform()\n\n");
  int retval = 0;
  json_parce parser;

  size_t filesize;
  char *filename;
  char *path = "./test_transform";
  char filepath[1024] = {0};

  json_parce_callbacks deep_cbs = {.on_array_value = test_JSONTestSuite_array,
                                   .on_object_key_value_pair =
                                       test_JSONTestSuite_object};

#ifdef _WIN32
  WIN32_FIND_DATA fd;
  HANDLE hFind = NULL;

  sprintf(filepath, "%s\\*.json", path);

  if ((hFind = FindFirstFile(filepath, &fd)) == INVALID_HANDLE_VALUE) {
    fprintf(stderr, "Failed to open directory \"%s\"", path);
    return -1;
  }

  do {
    if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      // skip directories
      continue;
    }

    sprintf(filepath, "%s\\%s", path, fd.cFileName);
    filesize = fd.nFileSizeLow;
    filename = fd.cFileName;
#else
  DIR *d = opendir(path);

  if (!d) {
    fprintf(stderr, "Failed to open directory \"%s\"\n", path);
    return -1;
  }

  struct dirent *entry;

  while ((entry = readdir(d)) != NULL) {
#ifdef _DIRENT_HAVE_D_TYPE
    if (entry->d_type != DT_REG) {
      // skip anything that's not a normal file.
      continue;
    }
#endif
    sprintf(filepath, "%s/%s", path, entry->d_name);

    struct stat sb = {0};
    if (stat(filepath, &sb) != 0) {
      fprintf(stderr, "Failed to get file info using stat. \"%s\"\n", filepath);
      return -1;
    }

    filesize = sb.st_size;
    filename = entry->d_name;
#endif // WIN32

    test_testname_string = filename;

    json_parce_init(&parser);
    size_t retval = json_deep_parce_execute_file(&parser, &deep_cbs, filepath);

    if (parser.err || (filesize != retval)) {
      fprintf(stderr, "\nERROR: \"%s\" FAILED\n", filepath);
      if (filesize != retval) {
        fprintf(stderr, "\texpected: %zd, actual: %zd\n", filesize, retval);
      }
      if (parser.err) {
        fprintf(stderr, "\t%s (%d) \n", json_errno_messages[parser.err],
                parser.err);
        fprintf(stderr, "\t%s(%d)\n\n", parser.file, parser.line);
      }
      retval++;
    }

    // free parser in case of failure.
    json_parce_free(&parser);
#ifdef _WIN32
  } while (FindNextFile(hFind, &fd));
  FindClose(hFind);
#else
  } // while loop
  closedir(d);
#endif // WIN32

  return retval;
}

#define TEST_UNICODE(INPUT, EXPECTED) \
status = process_unicode_escape_string((INPUT), &output); \
assert(status == 0); \
printf("test_process_unicode_string - Expecting <%s> == <%s>\n", output, (EXPECTED)); \
assert(strcmp(output, (EXPECTED)) == 0); \
free(output)

#define TEST_UNICODE_ERROR(INPUT, EXPECTED) \
status = process_unicode_escape_string((INPUT), &output); \
printf("test_process_unicode_string - Expecting status <%d> == <%d>\n", status, (EXPECTED)); \
assert(status == EXPECTED);

int test_process_unicode_string() {
  int status;
  char* output = NULL;
  printf("\n\ntest_process_unicode_string\n\n");

  TEST_UNICODE("Hello world!", "Hello world!");
  TEST_UNICODE("\\uFFEE", "\uFFEE");
  TEST_UNICODE("\\u0000", "");
  TEST_UNICODE("\\u0021", "!");
  TEST_UNICODE("\\u00FF\\u0021", "\u00FF!");
  TEST_UNICODE("\\uFFE8\\uFFE8\\uFFE8\\uFFE8\\uFFE8\\uFFE8\\uFFE8", "\uFFE8\uFFE8\uFFE8\uFFE8\uFFE8\uFFE8\uFFE8");
  TEST_UNICODE("\\uD83D\\uDE00", "\U0001F600");
  TEST_UNICODE("\\uD888\\uDFAF", "\U000323AF");
  // TEST_UNICODE("\\uD888\\uDFAF", "\U000323AF");
  TEST_UNICODE("\\uDBFF\\uDFFD", "\U0010FFFD");
  TEST_UNICODE("\\uDB40\\uDDEF", "\U000E01EF");
  TEST_UNICODE("\\uD884\\uDF4A", "\U0003134a");
  TEST_UNICODE("\\uD87E\\uDDF4", "\U0002F9F4");
  TEST_UNICODE("\\uD800\\uDC00", "\U00010000");

  TEST_UNICODE("\\uD800\\uDC00\\uFFE8", "\U00010000\uFFE8");
  TEST_UNICODE("!!\\uD800\\uDC00\\uFFE8!!", "!!\U00010000\uFFE8!!");
  TEST_UNICODE("!!\\uD800\\uDC000\\uFFE8!!", "!!\U000100000\uFFE8!!");

  TEST_UNICODE_ERROR("\\uD800!\\uDC00", -1); // character between high and low surrogates.
  TEST_UNICODE_ERROR("\\uD800", -1); // lonely high surrogate
  TEST_UNICODE_ERROR("\\uDC00", -1); // lonely low surrogate
  TEST_UNICODE_ERROR("\\uD800\\uD800", -2); // two high surrogate
  TEST_UNICODE_ERROR("\\uD800\\u0021", -2); // high surrogate + utf-8 character.
  TEST_UNICODE_ERROR("\\u0021\\uD800", -1); // utf-8 character + high surrogate.
  TEST_UNICODE_ERROR("\\u0021\\uDC00", -1); // utf-8 character + low surrogate.

  return 0;
}