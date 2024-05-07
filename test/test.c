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

static const char array_data[] =
    "[null,true,false,\"str\\\\ing\",{\"\":\"\\\\\"},[\"str\\\\\"],-1.23e15]";
static const size_t array_data_len = sizeof(array_data) - 1;

static const char object_data[] =
    "{\"\\\\\":\"\", \"null\": null, \"true\": true, \"false\": false, "
    "\"integer\": 1234,\n\t"
    "\"number\":-10.35E1234   , \"string\": \"str\\\\ing\", "
    "\"object\":{\"\\\\\":\"\\\\\"}, \"array\":[\"\\\\\"]}";
static const size_t object_data_len = sizeof(object_data) - 1;

static void get_log_string(const size_t maxlen, const char *str, size_t len,
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

int on_object_value_cb(json_parser *parser, const char *key, size_t key_length,
                       JSON_TYPE type, const char *value, size_t value_length) {
  return 0;
}

int on_array_value_cb(json_parser *parser, unsigned int index, JSON_TYPE type,
                      const char *value, size_t value_length) {
  return 0;
}

int on_deep_array_value_cb(json_parser *parser, unsigned int index,
                           JSON_TYPE type, const char *value,
                           size_t value_length) {
  return 0;
}

int on_deep_object_key_value_pair_cb(json_parser *parser, const char *key,
                                     size_t key_length, JSON_TYPE type,
                                     const char *value, size_t value_length) {
  return 0;
}

static void print_retval(size_t retval, size_t expected, json_parser *parser) {
  static int calls = 0;
  calls++;
  printf("%d) retval: %zd, expected: %zd\n", calls, retval, expected);
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

int test_parsing();
int test_JSONTestSuite();

int main() {
  json_parser parser;

  json_parser_callbacks cbs = {.on_array_value = on_array_value_cb,
                               .on_object_key_value_pair = on_object_value_cb};

  json_parser_callbacks deep_cbs = {.on_array_value = on_deep_array_value_cb,
                                    .on_object_key_value_pair =
                                        on_deep_object_key_value_pair_cb};

  // Test array
  json_parser_init(&parser);
  size_t retval =
      json_parser_execute(&parser, &cbs, array_data, array_data_len);
  print_retval(retval, array_data_len, &parser);

  // Test object
  json_parser_init(&parser);
  retval = json_parser_execute(&parser, &cbs, object_data, object_data_len);
  print_retval(retval, object_data_len, &parser);

  // Test deep array
  json_parser_init(&parser);
  retval =
      json_deep_parser_execute(&parser, &deep_cbs, array_data, array_data_len);
  print_retval(retval, array_data_len, &parser);
  json_parser_free(&parser);

  // Test deep object
  json_parser_init(&parser);
  retval = json_deep_parser_execute(&parser, &deep_cbs, object_data,
                                    object_data_len);
  print_retval(retval, object_data_len, &parser);
  json_parser_free(&parser);

  // 1
  json_parser_init(&parser);
  retval = json_parser_execute_file(&parser, &cbs, "./encodings/utf-8.json");
  print_retval(retval, 48, &parser);

  // 2
  json_parser_init(&parser);
  retval = json_parser_execute_file(&parser, &cbs, "./encodings/utf-8BOM.json");
  print_retval(retval, 51, &parser);

  // 3
  json_parser_init(&parser);
  retval = json_parser_execute_file(&parser, &cbs, "./encodings/utf-16LE.json");
  print_retval(retval, 48, &parser);

  // 4
  json_parser_init(&parser);
  retval = json_parser_execute_file(&parser, &cbs, "./encodings/utf-16BE.json");
  print_retval(retval, 48, &parser);

  // 5
  json_parser_init(&parser);
  retval =
      json_parser_execute_file(&parser, &cbs, "./encodings/utf-16LEBOM.json");
  print_retval(retval, 51, &parser);

  // 6
  json_parser_init(&parser);
  retval =
      json_parser_execute_file(&parser, &cbs, "./encodings/utf-16BEBOM.json");
  print_retval(retval, 51, &parser);

  // 7
  json_parser_init(&parser);
  retval = json_parser_execute_file(&parser, &cbs, "./encodings/utf-32LE.json");
  print_retval(retval, 48, &parser);

  // 8
  json_parser_init(&parser);
  retval = json_parser_execute_file(&parser, &cbs, "./encodings/utf-32BE.json");
  print_retval(retval, 48, &parser);

  // 9
  json_parser_init(&parser);
  retval =
      json_parser_execute_file(&parser, &cbs, "./encodings/utf-32LEBOM.json");
  print_retval(retval, 51, &parser);

  // 10
  json_parser_init(&parser);
  retval =
      json_parser_execute_file(&parser, &cbs, "./encodings/utf-32BEBOM.json");
  print_retval(retval, 51, &parser);

  retval = test_JSONTestSuite();
  assert(retval == 0);

  test_parsing();
}

static int test_parsing_on_array_value_cb(json_parser *parser,
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

static int test_parsing_on_object_value_cb(json_parser *parser, const char *key,
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
  json_parser_callbacks cbs = {.on_array_value = test_parsing_on_array_value_cb,
                               .on_object_key_value_pair =
                                   test_parsing_on_object_value_cb};

  json_parser parser;
  json_parser_init(&parser);

  size_t retval =
      json_parser_execute(&parser, &cbs, object_data, object_data_len);

  assert(retval == object_data_len);
  assert(parser.err == 0);

  print_retval(retval, object_data_len, &parser);

  return 0;
}

static char *test_JSONTestSuite_file = NULL;

int test_JSONTestSuite_array(json_parser *parser, unsigned int index,
                             JSON_TYPE type, const char *value,
                             size_t value_length) {
#ifdef JSON_PARCE_ENABLE_TEST_DEBUG
  printf("%65s", test_JSONTestSuite_file);
  printf("%10d", parser->current_depth->depth);
  if (parser->current_depth->key) {
    printf("%10.*s", (int)parser->current_depth->key_len,
           parser->current_depth->key);
  } else if (parser->current_depth->depth > 0) {
    printf("%10d", (int)parser->current_depth->array_index);
  } else {
    printf("%10s", "[ROOT]");
  }
  char *log_value = NULL;
  size_t log_value_len = 0;
  get_log_string(10, value, value_length, &log_value, &log_value_len);
  printf("%10s%10d%10d%15.*s\n", "-", index, type, (int)log_value_len,
         log_value);
  free(log_value);
#endif
  return 0;
}

int test_JSONTestSuite_object(json_parser *parser, const char *key,
                              size_t key_length, JSON_TYPE type,
                              const char *value, size_t value_length) {
#ifdef JSON_PARCE_ENABLE_TEST_DEBUG
  printf("%65s", test_JSONTestSuite_file);
  printf("%10d", parser->current_depth->depth);
  if (parser->current_depth->key) {
    printf("%10.*s", (int)parser->current_depth->key_len,
           parser->current_depth->key);
  } else if (parser->current_depth->depth > 0) {
    printf("%10d", (int)parser->current_depth->array_index);
  } else {
    printf("%10s", "{ROOT}");
  }
  char *log_value = NULL;
  size_t log_value_len = 0;
  get_log_string(10, value, value_length, &log_value, &log_value_len);
  printf("%10.*s%10s%10d%15.*s\n", (int)key_length, key, "-", type,
         (int)log_value_len, log_value);
  free(log_value);
#endif
  return 0;
}

int test_JSONTestSuite() {
  int retval = 0;
  json_parser parser;

  size_t filesize;
  char *filename;
  char *path = "./test_parsing";
  char filepath[1024] = {0};

  json_parser_callbacks deep_cbs = {.on_array_value = test_JSONTestSuite_array,
                                    .on_object_key_value_pair =
                                        test_JSONTestSuite_object};

#ifdef JSON_PARCE_ENABLE_TEST_DEBUG
  printf("%65s%10s%10s%10s%10s%10s%15s\n", "Filename", "Depth", "Parent", "Key",
         "Index", "Type", "Value");
#endif

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

    test_JSONTestSuite_file = filename;

    json_parser_init(&parser);
    size_t retval = json_deep_parser_execute_file(&parser, &deep_cbs, filepath);

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
          fprintf(stderr, "\t%s.%s(%d)\n\n", parser.func, parser.file,
                  parser.line);
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
          fprintf(stderr, "\t%s.%s(%d)\n\n", parser.func, parser.file,
                  parser.line);
        }
      }
    } else if (strncmp(filename, "n_", 2) == 0) {
      static const char *ignore_files[] = {
          // currently not validating numerics
          "n_array_just_minus.json",
          "n_number_++.json",
          "n_number_+1.json",
          "n_number_-01.json",
          "n_number_-1.0..json",
          "n_number_-2..json",
          "n_number_.-1.json",
          "n_number_.2e-3.json",
          "n_number_0.1.2.json",
          "n_number_0.3e+.json",
          "n_number_0.3e.json",
          "n_number_0.e1.json",
          "n_number_0e+.json",
          "n_number_0e.json",
          "n_number_0_capital_E+.json",
          "n_number_0_capital_E.json",
          "n_number_1.0e+.json",
          "n_number_1.0e-.json",
          "n_number_1.0e.json",
          "n_number_1eE2.json",
          "n_number_2.e+3.json",
          "n_number_2.e-3.json",
          "n_number_2.e3.json",
          "n_number_9.e+.json",
          "n_number_expression.json",
          "n_number_invalid+-.json",
          "n_number_neg_int_starting_with_zero.json",
          "n_number_neg_real_without_int_part.json",
          "n_number_real_without_fractional_part.json",
          "n_number_starting_with_dot.json",
          "n_number_with_leading_zero.json",
          "n_string_1_surrogate_then_escape_u.json",
          "n_string_1_surrogate_then_escape_u1.json",
          "n_string_1_surrogate_then_escape_u1x.json",
          "n_string_escaped_ctrl_char_tab.json",
          "n_string_escaped_emoji.json",
          "n_string_escape_x.json",
          "n_string_incomplete_escaped_character.json",
          "n_string_incomplete_surrogate.json",
          "n_string_incomplete_surrogate_escape_invalid.json",
          "n_string_invalid-utf-8-in-escape.json",
          "n_string_invalid_backslash_esc.json",
          "n_string_invalid_unicode_escape.json",
          "n_string_invalid_utf8_after_escape.json",
          "n_string_unescaped_newline.json",
          "n_string_unescaped_tab.json"};
      int ignore = 0;
      for (int i = 0; i < (sizeof(ignore_files) / sizeof(*ignore_files)); ++i) {
        if (strcmp(ignore_files[i], filename) == 0) {
          ignore = 1;
          break;
        }
      }

      if (!ignore && !parser.err) {
#ifdef JSON_PARCE_STRICT_MODE
        // fprintf(stderr, "\"%s\",\n", filename);
        fprintf(stderr, "\nERROR: \"%s\" SUCCESS\n", filepath);
        retval++;
#else
        printf("\nINFO: \"%s\" SUCCESS - (JSON_PARCE_STRICT_MODE is disabled)",
               filepath);
#endif
      } else if (ignore && parser.err) {
        printf("\nINFO: \"%s\" FAILED successfully but ignored\n", filepath);
      }
    } else {
      fprintf(stderr, "\nWARNING: Unexpected file parsed. %s", filepath);
    }
    // free parser in case of failure.
    json_parser_free(&parser);
#ifdef _WIN32
  } while (FindNextFile(hFind, &fd));
  FindClose(hFind);
#else
  } // while loop
  closedir(d);
#endif // WIN32

  return retval;
}
