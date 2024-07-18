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

static char *test_testname_string;

static int test_parsing(char *path);
static int test_transform(char *path);
static void validate_value(JSON_TYPE type, const char *value,
                           size_t value_length);

int main(int argc, char **argv) {
  if (argc != 3 && argc != 2) {
    fprintf(stderr, "usage:\n %s <test_parsing dir> [<test_transform dir>]",
            argv[0]);
    exit(1);
  }

#ifdef _WIN32
  if (!setlocale(LC_ALL, ".UTF-8")) {
    fprintf(stderr, "setlocale failed!\n");
  }
#else
  if (!setlocale(LC_ALL, "C.utf8")) {
    fprintf(stderr, "setlocale failed!\n");
  }
#endif

  int status = test_parsing(argv[1]);

  if (argc == 3) {
    status &= test_transform(argv[2]);
  }

  return status;
}

int test_on_end(json_parce *parser, JSON_TYPE type) { return 0; }

int test_on_start(json_parce *parser, JSON_TYPE type) { return 0; }

int test_JSONTestSuite_array(json_parce *parser, size_t index, JSON_TYPE type,
                             const char *value, size_t value_length) {
  validate_value(type, value, value_length);
  return 0;
}

int test_JSONTestSuite_object(json_parce *parser, const char *key,
                              size_t key_len, JSON_TYPE type, const char *value,
                              size_t value_length) {
  validate_value(type, value, value_length);
  return 0;
}

int test_parsing(char *path) {
  printf("\n\n test_parsing\n\n");
  int status = 0;
  json_parce parser;

  size_t filesize;
  char *filename;
  char filepath[1024] = {0};

  json_parce_callbacks cbs = {.on_array_value = test_JSONTestSuite_array,
                              .on_object_key_value_pair =
                                  test_JSONTestSuite_object,
                              .on_end = test_on_end,
                              .on_start = test_on_start};

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
    size_t retval = json_deep_parce_execute_file(&parser, &cbs, filepath);

    // y_* files should work
    // i_* files may work.
    // n_* files should cause failures.
    if (strncmp(filename, "y_", 2) == 0) {
      // static const char *ignore_files[] = {};
      // The following files are all tests that test values that aren't in
      // objects or arrays.
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

      printf("file: %s, size: %zd, error: %d\n", filepath, filesize, parser.err);
      if (!ignore && (parser.err || (filesize != retval))) {
        fprintf(stderr, "\nERROR: \"%s\" FAILED\n", filepath);
        if (filesize != retval) {
          fprintf(stderr, "\t%d: expected: %zd, actual: %zd\n", __LINE__, filesize, retval);
        }
        if (parser.err) {
          fprintf(stderr, "\t%s (%d) \n", json_errno_messages[parser.err],
                  parser.err);
          fprintf(stderr, "\t%s(%d)\n\n", parser.file, parser.line);
        }
        status++;
      } else if (ignore && !parser.err) {
        printf("\nINFO: \"%s\" COMPLETED successfully but ignored\n", filepath);
      }

    } else if (strncmp(filename, "i_", 2) == 0) {
      // Exclude the utf16 tests from the size check.
      if (parser.err ||
          (filesize != retval && (strstr(filename, "16") == NULL))) {
        fprintf(stderr, "\nWARNING: \"%s\" FAILED\n", filepath);
        if (filesize != retval) {
          fprintf(stderr, "\t%d: expected: %zd, actual: %zd\n", __LINE__, filesize, retval);
        }
        if (parser.err) {
          fprintf(stderr, "\t%s (%d) \n", json_errno_messages[parser.err],
                  parser.err);
          fprintf(stderr, "\t%s(%d) - %s()\n\n", parser.file, parser.line,
                  parser.func);
        }
      }
    } else if (strncmp(filename, "n_", 2) == 0) {
      if (!parser.err) {
#ifdef JSON_PARCE_STRICT_MODE
        // fprintf(stderr, "\"%s\",\n", filename);
        fprintf(stderr, "\nERROR: \"%s\" SUCCESS\n", filepath);
        status++;
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

  return status;
}

int test_transform_on_end(json_parce *parser, JSON_TYPE type) { return 0; }

int test_transform_on_start(json_parce *parser, JSON_TYPE type) { return 0; }

int test_transform_array(json_parce *parser, size_t index, JSON_TYPE type,
                         const char *value, size_t value_length) {
  printf("[%s] - \"%*.s\" -> ", test_testname_string, (int)value_length, value);
  switch (type) {
  case NUMBER: {
    json_parce_int_t rint;
    json_parce_real_t real;
    int printed = 0;
    int ret = json_parce_int(value, value_length, &rint);
    if (ret != 0) {
      ret = json_parce_real(value, value_length, &real);
    } else {
      printed = 1;
      printf("(json_parce_int_t) %lld\n", rint);
    }
    if (ret != 0) {
      fprintf(stderr, "\n [%s] - Failed to parse number: <%.*s>\n",
              test_testname_string, (int)value_length, value);
    } else if (!printed) {
      printf("(json_parce_real_t) %lf\n", real);
    }
    break;
  }
  case STRING: {
    char *str = json_parce_string(value, value_length);
    if (str != NULL) {
      printf("%s\n", str);
      free(str);
    } else {
      fprintf(stderr, "\n [%s] - Failed to parse string: <%.*s>\n",
              test_testname_string, (int)value_length, value);
    }
    break;
  }
  case BOOL_TYPE: {
    int ret = json_parce_bool(value);
    printf("%s\n", (ret ? "true" : "false"));
    assert((ret == 0) || (ret == 1));
    break;
  }
  case OBJECT:
  case ARRAY:
  case NONE:
    fprintf(stderr, "\n [%s] - Invalid value type.", test_testname_string);
    assert(0);
    break;
  case NULL_TYPE:
    break;
  }
  return 0;
}

int test_transform_object(json_parce *parser, const char *key, size_t key_len,
                          JSON_TYPE type, const char *value,
                          size_t value_length) {
  printf("[%s] - \"%*.s\" -> ", test_testname_string, (int)value_length, value);
  switch (type) {
  case NUMBER: {
    json_parce_int_t rint;
    json_parce_real_t real;
    int printed = 0;
    int ret = json_parce_int(value, value_length, &rint);
    if (ret != 0) {
      ret = json_parce_real(value, value_length, &real);
    } else {
      printed = 1;
      printf("(json_parce_int_t) %lld\n", rint);
    }
    if (ret != 0) {
      fprintf(stderr, "\n [%s] - Failed to parse number: <%.*s>\n",
              test_testname_string, (int)value_length, value);
    } else if (!printed) {
      printf("(json_parce_real_t) %lf\n", real);
    }
    break;
  }
  case STRING: {
    char *str = json_parce_string(value, value_length);
    if (str != NULL) {
      printf("%s\n", str);
      free(str);
    } else {
      fprintf(stderr, "\n [%s] - Failed to parse string: <%.*s>\n",
              test_testname_string, (int)value_length, value);
    }
    break;
  }
  case BOOL_TYPE: {
    int ret = json_parce_bool(value);
    printf("%s\n", (ret ? "true" : "false"));
    assert((ret == 0) || (ret == 1));
    break;
  }
  case OBJECT:
  case ARRAY:
  case NONE:
    fprintf(stderr, "\n [%s] - Invalid value type.", test_testname_string);
    assert(0);
    break;
  case NULL_TYPE:
    break;
  }
  return 0;
}

int test_transform(char *path) {
  printf("\n\n test_transform\n\n");
  int retval = 0;
  json_parce parser;

  size_t filesize;
  char *filename;
  char filepath[1024] = {0};

  json_parce_callbacks cbs = {.on_array_value = test_transform_array,
                              .on_object_key_value_pair = test_transform_object,
                              .on_end = test_transform_on_end,
                              .on_start = test_transform_on_start};

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
    size_t retval = json_deep_parce_execute_file(&parser, &cbs, filepath);

    if (parser.err || (filesize != retval)) {
      fprintf(stderr, "\nERROR: \"%s\" FAILED\n", filepath);
      if (filesize != retval) {
        fprintf(stderr, "\t%d: expected: %zd, actual: %zd\n", __LINE__, filesize, retval);
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
      fprintf(stderr, "\n [%s] - Failed to parse number: <%.*s>\n",
              test_testname_string, (int)value_length, value);
    }
    break;
  }
  case STRING: {
    char *str = json_parce_string(value, value_length);
    if (str != NULL) {
      free(str);
    } else {
      fprintf(stderr, "\n [%s] - Failed to parse string: <%.*s>\n",
              test_testname_string, (int)value_length, value);
    }
    break;
  }
  case BOOL_TYPE: {
    int ret = json_parce_bool(value);
    assert((ret == 0) || (ret == 1));
    break;
  }
  case NONE:
    fprintf(stderr, "\n [%s] - Invalid value type.", test_testname_string);
    assert(0);
    break;
  case OBJECT:
  case ARRAY:
  case NULL_TYPE:
    break;
  }
}