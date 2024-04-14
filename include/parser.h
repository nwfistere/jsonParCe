#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>

/*
 * Map for errno-related constants.
 */
#define ERRNO_MAP(XX)                                                          \
  XX(OK, "success")                                                            \
  XX(INVALID_STATE, "invalid state")                                           \
  XX(UNKNOWN, "unknown error")                                                 \
  XX(CALLBACK_FAILED, "callback failed (returned non-zero)")                   \
  XX(INVALID_CHARACTER, "invalid character found")

#define ERRNO_GEN(n, s) ERRNO_##n,
enum json_errno { ERRNO_MAP(ERRNO_GEN) };
#undef ERRNO_GEN

typedef struct json_parser {
  unsigned int state;
  unsigned int err;
  size_t nread;
  // const char **object_marks;
  // unsigned int object_marks_len;
  // unsigned int object_marks_size;
  // const char **array_marks;
  // unsigned int array_marks_len;
  // unsigned int array_marks_size;
  const char *array_item_mark;
  unsigned int array_index;
  unsigned int array_count;
  unsigned int object_count;
  const char *object_key_mark;
  unsigned int object_key_len;
  const char *object_value_mark;
  unsigned int object_value_len;
} json_parser;

typedef int (*json_object_cb)(json_parser *, const char *key,
                              unsigned int key_length, const char *value,
                              unsigned int value_length);
typedef int (*json_array_cb)(json_parser *, unsigned int index,
                             const char *value, size_t value_length);
typedef int (*json_cb)(json_parser *);

// return 0 on success, 1 on failure.
typedef struct json_parser_callbacks {
  json_object_cb on_object_key_value_pair;
  json_array_cb on_array_value;
} json_parser_callbacks;

void json_parser_init(json_parser *parser);
void json_parser_callbacks_init(json_parser_callbacks *parser);

size_t json_parser_execute(json_parser *parser,
                           json_parser_callbacks *callbacks, const char *data,
                           size_t len);

#endif // PARSER_H