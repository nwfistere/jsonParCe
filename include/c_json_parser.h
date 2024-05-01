#ifndef PARSER_H
#define PARSER_H
#include <uchar.h>
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

// Could hold start of the object/array so we could get the whole thing...
typedef struct json_depth {
  unsigned int type : 1; // 0 is array, 1 is object
  unsigned int depth;
  unsigned int array_index;
  const char *key;
  size_t key_len;
  struct json_depth *parent;
} json_depth;

typedef struct json_parser {
  size_t nread;
  size_t object_key_len;
  unsigned int state; // 6 bits
  unsigned int err;   // 4 bits
  unsigned int array_index;
  unsigned int array_count;
  unsigned int object_count;
  // unsigned int encoding; // Don't think we'll need this right now...
  // Currently translating everythign to mb utf8

  const char *array_item_mark;
  const char *object_key_mark;
  const char *object_value_mark;
  void *data;

  // deep parser members
  json_depth *current_depth;
  unsigned int
      max_depth; // Max depth to parse into, will return whole child json object
                 // at this point. (zero means no max depth.)

  // debug help
  const char *file;
  int line;
  const char *func;
} json_parser;

typedef int (*json_object_cb)(json_parser *, const char *key, size_t key_len,
                              const char *value, size_t value_length);
typedef int (*json_array_cb)(json_parser *, unsigned int index,
                             const char *value, size_t value_length);
typedef int (*json_object_typed_cb)(json_parser *, const char *key,
                                    size_t key_len, JSON_TYPE type,
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

void json_depth_init(json_depth *depth);

LIBRARY_API void json_parser_init(json_parser *parser);
LIBRARY_API void json_parser_callbacks_init(json_parser_callbacks *parser);

LIBRARY_API size_t json_parser_execute(json_parser *parser,
                                       json_parser_callbacks *callbacks,
                                       const char *data, size_t len);

LIBRARY_API size_t json_deep_parser_execute(json_parser *parser,
                                            json_parser_callbacks *callbacks,
                                            const char *data, size_t len);

LIBRARY_API size_t json_parser_execute_utf16(json_parser *parser,
                                             json_parser_callbacks *callbacks,
                                             const char16_t *data, size_t len);

LIBRARY_API size_t json_parser_execute_utf32(json_parser *parser,
                                             json_parser_callbacks *callbacks,
                                             const char32_t *data, size_t len);

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
