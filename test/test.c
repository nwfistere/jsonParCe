#include "c_json_parser.h"
#include "string.h"
#include <assert.h>
#include <locale.h>
#include <stdio.h>

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
    "\"number\":-10.35E1234   , \"string\": \"str\\\\ing\", "
    "\"object\":{\"\\\\\":\"\\\\\"}, \"array\":[\"\\\\\"]}";
static const size_t object_data_len = sizeof(object_data) - 1;

int on_array_value_cb(json_parser *parser, unsigned int index,
                      const char *value, size_t value_length) {
#ifdef _WIN32
  SetConsoleOutputCP(65001); // Set windows output to utf-8
#endif
  printf("[%d]:<%.*s>\n", index, value_length, value);
  return 0;
}

int on_object_key_value_pair_cb(json_parser *parser, const char *key,
                                size_t key_length, const char *value,
                                size_t value_length) {
#ifdef _WIN32
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

int on_deep_array_value_cb(json_parser *parser, unsigned int index,
                           const char *value, size_t value_length) {
#ifdef _WIN32
  SetConsoleOutputCP(65001); // Set windows output to utf-8
#endif
  printf("%s: %d [%d]:<%.*s>\n", "on_deep_array_value_cb",
         parser->current_depth->depth, index, value_length, value);
  return 0;
}

int on_deep_object_key_value_pair_cb(json_parser *parser, const char *key,
                                     size_t key_length, const char *value,
                                     size_t value_length) {
#ifdef _WIN32
  SetConsoleOutputCP(65001); // Set windows output to utf-8
#endif
  printf("%s: %d \"%.*s\": <%.*s>\n", "on_deep_object_key_value_pair_cb",
         parser->current_depth->depth, key_length, key, value_length, value);
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

int test_JSONTestSuite();

int main() {
  json_parser parser;

  json_parser_callbacks_typed tcbs = {.on_array_value = on_typed_array_value_cb,
                                      .on_object_key_value_pair =
                                          on_typed_object_value_cb};

  json_parser_callbacks cbs = {.on_array_value = on_array_value_cb,
                               .on_object_key_value_pair =
                                   on_object_key_value_pair_cb};

  json_parser_callbacks deep_cbs = {.on_array_value = on_deep_array_value_cb,
                                    .on_object_key_value_pair =
                                        on_deep_object_key_value_pair_cb};

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

  // Test deep array
  json_parser_init(&parser);
  retval =
      json_deep_parser_execute(&parser, &deep_cbs, array_data, array_data_len);
  assert(retval == array_data_len);

  // Test deep object
  json_parser_init(&parser);
  retval = json_deep_parser_execute(&parser, &deep_cbs, object_data,
                                    object_data_len);
  assert(retval == object_data_len);

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
}

int test_JSONTestSuite() {
  int retval = 0;
  json_parser parser;
  json_parser_callbacks cbs = {0};

  size_t filesize;
  char *filename;
  char *path = "./test_parsing";
  char filepath[1024] = {0};

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

    // Skip the failures for now, focus on the y_ and i_ prefixed files.
    // y_* files should work
    // i_* files may work.
    if (strncmp(fd.cFileName, "n_", 2) == 0) {
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

    json_parser_init(&parser);
    size_t retval = json_parser_execute_file(&parser, &cbs, filepath);

    if (strncmp(filename, "y_", 2) == 0) {
      if (parser.err || filesize != retval) {
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
    }

#ifdef _WIN32

  } while (FindNextFile(hFind, &fd));

  FindClose(hFind);
#else
  } // while loop
  closedir(d);
#endif // WIN32

  return retval;
}
