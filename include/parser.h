#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>

/*
 * Map for errno-related constants.
 */
#define ERRNO_MAP(XX)                                                          \
  XX(OK, "success")                                                            \
  XX(INVALID_STATE, "invalid state")                                           \
  XX(UNKNOWN, "unknown error") \
  XX(MALLOC_FAILED, "malloc failed")

#define ERRNO_GEN(n, s) ERRNO_##n,
enum json_errno { ERRNO_MAP(ERRNO_GEN) };
#undef ERRNO_GEN

typedef struct json_parser {
  unsigned int state;
  unsigned int err;
  size_t nread;
  const char** object_marks;
  unsigned int object_marks_len;
  unsigned int object_marks_size;
  const char** array_marks;
  unsigned int array_marks_len;
  unsigned int array_marks_size;
  const char*** array_item_marks;
} json_parser;

typedef int (*json_data_cb)(json_parser *, const char *key, size_t *key_length,
                            const char *value, size_t value_length);
typedef int (*json_cb)(json_parser *);

typedef struct json_parser_callbacks {
  json_data_cb on_key_value_pair;
} json_parser_callbacks;

void json_parser_init(json_parser *parser);
void json_parser_callbacks_init(json_parser_callbacks *parser);

size_t json_parser_execute(json_parser *parser,
                           json_parser_callbacks *callbacks, const char *data,
                           size_t len);

#endif // PARSER_H