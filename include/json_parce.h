#ifndef JSON_PARCE_H
#define JSON_PARCE_H

#include <float.h>
#include <limits.h>
#include <stddef.h>
#include <uchar.h>

#ifdef __cplusplus
extern "C" {
#endif

// 255
#define JSON_PARCE_DONE_STATE ((1 << 8) - 1)

typedef enum JSON_TYPE {
  NONE = 0,
  OBJECT = 1 << 0,
  ARRAY = 1 << 1,
  NUMBER = 1 << 2,
  STRING = 1 << 3,
  BOOL_TYPE = 1 << 4,
  NULL_TYPE = 1 << 5
} JSON_TYPE;

#ifdef _WIN32
#ifdef JSON_PARCE_LIBRARY_EXPORTS
#define JSON_PARCE_API __declspec(dllexport)
#elif defined(JSON_PARCE_STATIC_LIBRARY)
#define JSON_PARCE_API
#else
#define JSON_PARCE_API __declspec(dllimport)
#endif
#else
#define JSON_PARCE_API
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
  XX(INVALID_ENCODING, "invalid encoding")                                     \
  XX(INCOMPLETE_DATA, "buffer ended before end of json")                       \
  XX(CALLBACK_REQUESTED_STOP, "stop requested by callback")                    \
  XX(OUT_OF_RANGE, "value is out of range")                                    \
  XX(INVALID_OBJECT_KEY, "invalid object key")

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
  size_t array_index;
  const char *key;
  size_t key_len;
  const char *json_pointer;
  const char *json_path;
  struct json_depth *parent;
} json_depth;

typedef struct json_parce {
  size_t nread;
  size_t object_key_len;
  size_t array_index;
  unsigned int state; // 6 bits
  unsigned int err;   // 4 bits
  unsigned int array_count;
  unsigned int object_count;

  const char *array_item_mark;
  const char *object_key_mark;
  const char *object_value_mark;
  void *data;

  // deep parser members
  json_depth *current_depth;

  // Max depth to parse into, will return whole child json object
  // at this point. (zero means no max depth.)
  unsigned int max_depth;

#ifdef JSON_PARCE_STRICT_MODE
  // Flags for the current state
  unsigned short fs_significand : 1;
  unsigned short fs_fraction : 1;
  unsigned short fs_exponent : 1;

  // Flags for the checks in the state.
  unsigned short f_minus : 1;
  unsigned short f_nonzero : 1;
  unsigned short f_zero : 1;
  unsigned short : 2;

  unsigned int return_state;
#endif

  // debug help
  const char *file;
  int line;
  const char *func;
} json_parce;

// CALLBACKS
// return 0 for success
// return 1 for failure
// return 2 for non-failure stop.
// This will stop parsing, but json_parce_execute can be recalled and
// continued.

typedef int (*json_notice_cb)(json_parce *, JSON_TYPE);

typedef int (*json_object_cb)(json_parce *parser, const char *key,
                              size_t key_len, JSON_TYPE type, const char *value,
                              size_t value_length);
typedef int (*json_array_cb)(json_parce *parser, size_t index, JSON_TYPE type,
                             const char *value, size_t value_length);

typedef struct json_parce_callbacks {
  json_object_cb on_object_key_value_pair;
  json_array_cb on_array_value;
  json_notice_cb on_start;
  json_notice_cb on_end;
} json_parce_callbacks;

JSON_PARCE_API void json_parce_init(json_parce *parser);

JSON_PARCE_API void json_parce_free(json_parce *parser);

JSON_PARCE_API size_t json_parce_execute(json_parce *parser,
                                         json_parce_callbacks *callbacks,
                                         const char *data, size_t len);

JSON_PARCE_API size_t json_deep_parce_execute(json_parce *parser,
                                              json_parce_callbacks *callbacks,
                                              const char *data, size_t len);

JSON_PARCE_API size_t json_parce_execute_utf16(json_parce *parser,
                                               json_parce_callbacks *callbacks,
                                               const char16_t *data,
                                               size_t len);

JSON_PARCE_API size_t json_parce_execute_utf32(json_parce *parser,
                                               json_parce_callbacks *callbacks,
                                               const char32_t *data,
                                               size_t len);

JSON_PARCE_API size_t json_parce_execute_file(json_parce *parser,
                                              json_parce_callbacks *callbacks,
                                              const char *file);

JSON_PARCE_API size_t json_parce_execute(json_parce *parser,
                                         json_parce_callbacks *callbacks,
                                         const char *data, size_t len);

JSON_PARCE_API size_t json_parce_execute_file(json_parce *parser,
                                              json_parce_callbacks *callbacks,
                                              const char *file);

JSON_PARCE_API size_t json_deep_parce_execute_file(
    json_parce *parser, json_parce_callbacks *callbacks, const char *file);

#ifndef JSON_PARCE_REAL
#define JSON_PARCE_REAL double
#define JSON_PARCE_REAL_MAX DBL_MAX
#define JSON_PARCE_REAL_MIN DBL_MIN
#define JSON_PARCE_REAL_MAX_DIG                                                \
  (3 + DBL_MANT_DIG - DBL_MIN_EXP) // Maximum number of characters
#endif

#ifndef JSON_PARCE_INT
#define JSON_PARCE_INT long long
#define JSON_PARCE_INT_MAX LLONG_MAX
#define JSON_PARCE_INT_MIN LLONG_MIN
#define JSON_PARCE_INT_MAX_DIG                                                 \
  ((3 * sizeof(JSON_PARCE_INT)) +                                              \
   2) // Roughly the maximum number of characters in JSON_PARCE_INT
#endif

typedef JSON_PARCE_REAL json_parce_real_t;
typedef JSON_PARCE_INT json_parce_int_t;

JSON_PARCE_API char *json_parce_string(const char *str, size_t len);
JSON_PARCE_API int json_parce_bool(const char *str);
JSON_PARCE_API int json_parce_real(const char *str, size_t len,
                                   json_parce_real_t *ret);
JSON_PARCE_API int json_parce_int(const char *str, size_t len,
                                  json_parce_int_t *ret);

#ifdef __cplusplus
}
#endif
#endif // JSON_PARCE_H
