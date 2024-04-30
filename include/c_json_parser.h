#ifndef PARSER_H
#define PARSER_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

typedef enum JSON_TYPE {
  NONE = 0,
  OBJECT,
  ARRAY,
  NUMBER,
  STRING,
  BOOL_TYPE,
  NULL_TYPE
} JSON_TYPE;

#ifdef _WIN32
#ifdef LIBRARY_EXPORTS
#define LIBRARY_API __declspec(dllexport)
#else
#ifdef LIBRARY_IMPORTS
#define LIBRARY_API __declspec(dllimport)
#else
#define LIBRARY_API
#endif
#endif
#else
#define LIBRARY_API
#endif

/*
 * Map for errno-related constants.
 */
#define ERRNO_MAP(XX)                                                          \
  XX(OK, "success")                                                            \
  XX(INVALID_STATE, "invalid state")                                           \
  XX(UNKNOWN, "unknown error")                                                 \
  XX(CALLBACK_FAILED, "callback failed (returned non-zero)")                   \
  XX(INVALID_CHARACTER, "invalid character found")                             \
  XX(FILE_OPEN_FAILURE, "failed to open file")                                 \
  XX(INVALID_ENCODING, "invalid encoding")

#define ERRNO_GEN(n, s) ERRNO_##n,
enum json_errno { ERRNO_MAP(ERRNO_GEN) };
#undef ERRNO_GEN

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif
#define ERRNO_MSG_GEN(n, s) s,
static const char *json_errno_messages[] = {ERRNO_MAP(ERRNO_MSG_GEN)};
#undef ERRNO_MSG_GEN
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

typedef struct json_parser {
  size_t nread;
  size_t object_key_len;
  unsigned int state;
  unsigned int err;
  unsigned int array_index;
  unsigned int array_count;
  unsigned int object_count;
  unsigned int encoding;

  const char *array_item_mark;
  const char *object_key_mark;
  const char *object_value_mark;
  void *data;

  // debug help
  const char *file;
  int line;
  const char *func;
} json_parser;

typedef int (*json_object_cb)(json_parser *, const char *key, size_t key_length,
                              const char *value, size_t value_length);
typedef int (*json_array_cb)(json_parser *, unsigned int index,
                             const char *value, size_t value_length);
typedef int (*json_object_typed_cb)(json_parser *, const char *key,
                                    size_t key_length, JSON_TYPE type,
                                    const char *value, size_t value_length);
typedef int (*json_array_typed_cb)(json_parser *, unsigned int index,
                                   JSON_TYPE type, const char *value,
                                   size_t value_length);
typedef int (*json_cb)(json_parser *);

// return 0 on success, 1 on failure.
typedef struct json_parser_callbacks {
  json_object_cb on_object_key_value_pair;
  json_array_cb on_array_value;
} json_parser_callbacks;

typedef struct json_parser_callbacks_typed {
  json_object_typed_cb on_object_key_value_pair;
  json_array_typed_cb on_array_value;
} json_parser_callbacks_typed;

LIBRARY_API void json_parser_init(json_parser *parser);
LIBRARY_API void json_parser_callbacks_init(json_parser_callbacks *parser);

LIBRARY_API size_t json_parser_execute(json_parser *parser,
                                       json_parser_callbacks *callbacks,
                                       const char *data, size_t len);

LIBRARY_API size_t json_parser_execute_file(json_parser *parser,
                                            json_parser_callbacks *callbacks,
                                            const char *file);

LIBRARY_API size_t json_parser_typed_execute(
    json_parser *parser, json_parser_callbacks_typed *callbacks,
    const char *data, size_t len);

LIBRARY_API size_t json_parser_typed_execute_file(
    json_parser *parser, json_parser_callbacks_typed *callbacks,
    const char *file);

#ifdef __cplusplus
}
#endif
#endif // PARSER_H
