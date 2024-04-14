#include "parser.h"

#include <stdlib.h>
#include <string.h>

#define SET_ERRNO(e)                                                           \
  do {                                                                         \
    parser->nread = nread;                                                     \
    parser->err = (e);                                                         \
  } while (0);

#define JSON_ERRNO(parser) ((enum json_errno)(parser->err))

// #define MARK_OBJECT_START(P) \
//   if (parser->object_marks_len == parser->object_marks_size) { \
//     const char** t_object_marks = realloc(parser->object_marks, 2 * parser->object_marks_size * sizeof(char*)); \
//     if (t_object_marks == NULL) { \
//       SET_ERRNO(ERRNO_MALLOC_FAILED); \
//       goto error; \
//     } \
//     memset(parser->object_marks + parser->object_marks_size, 0, parser->object_marks_size * sizeof(char*)); \
//     parser->object_marks = t_object_marks; \
//     parser->object_marks_size *= 2; \
//   } \
//   parser->object_marks[parser->object_marks_len] = P; \
//   parser->object_marks_len++;

// #define MARK_ARRAY_START(P) \
//   if (parser->array_marks_len == parser->array_marks_size) { \
//     const char** t_array_marks = realloc(parser->array_marks, 2 * parser->array_marks_size * sizeof(char*)); \
//     if (t_array_marks == NULL) { \
//       SET_ERRNO(ERRNO_MALLOC_FAILED); \
//       goto error; \
//     } \
//     memset(parser->array_marks + parser->array_marks_size, 0, parser->array_marks_size * sizeof(char*)); \
//     parser->array_marks = t_array_marks; \
//     parser->array_marks_size *= 2; \
//   } \
//   parser->array_marks[parser->array_marks_len] = P; \
//   parser->array_marks_len++;

#define MARK_ARRAY_ITEM_START(P) parser->array_item_mark = (P)

#define OB '['
#define CB ']'
#define OCB '{'
#define CCB '}'
#define CR '\r'
#define LF '\n'
#define QM '"'
#define COMMA ','
#define IS_NEWLINE(c) (c) == LF || (c) == CR
#define IS_WHITESPACE(c) (c) == ' ' || (c) == '\t' || (c) == LF || (c) == CR
#define UPDATE_STATE(V) p_state = (enum state)(V);
#define REEXECUTE() goto reexecute

enum state {
  s_dead = 1,
  s_start,
  s_parse_array,
  s_parse_array_string,
  s_parse_array_numeric,
  s_parse_array_n,
  s_parse_array_nu,
  s_parse_array_nul,
  s_parse_array_null,
  s_parse_array_t,
  s_parse_array_tr,
  s_parse_array_tru,
  s_parse_array_true,
  s_parse_array_f,
  s_parse_array_fa,
  s_parse_array_fal,
  s_parse_array_fals,
  s_parse_array_false,
  s_parse_array_find_array_end,
  s_parse_array_find_array_end_string_end,
  s_parse_array_find_object_end_string_end,
  s_parse_array_item_end,
  s_parse_array_find_object_end,
  s_start_object,
};

void json_parser_init(json_parser *parser) {
  memset(parser, 0, sizeof(*parser));
  parser->state = s_start;
}

size_t json_parser_execute(json_parser *parser,
                           json_parser_callbacks *callbacks, const char *data,
                           size_t len) {
  const char *p = data;
  const char *key = 0;
  const char *value = 0;
  enum state p_state = (enum state)parser->state;
  size_t nread = parser->nread;
  char ch;

  for (p = data; p != data + len; p++) {
    ch = *p;
  reexecute:
    switch (p_state) {
    case s_dead: {
      SET_ERRNO(ERRNO_INVALID_STATE);
      goto error;
    }
    case s_start: {
      if (IS_NEWLINE(ch)) {
        break;
      }
      if (ch == OB) {
        // MARK_ARRAY_START(p);
        UPDATE_STATE(s_parse_array);
      } else if (ch == OCB) {
        // MARK_OBJECT_START(p);
        UPDATE_STATE(s_start_object);
      }
      break;
    }
    case s_parse_array: {
      if (IS_WHITESPACE(ch) || ch == COMMA) {
        break;
      }
      MARK_ARRAY_ITEM_START(p);
      if (ch == OB) {
        UPDATE_STATE(s_parse_array_find_array_end);
        REEXECUTE();
      } else if (ch == OCB) {
        UPDATE_STATE(s_parse_array_find_object_end);
        REEXECUTE();
      } else if (ch == QM) {
        UPDATE_STATE(s_parse_array_string);
      } else if (ch == 't') {
        UPDATE_STATE(s_parse_array_tr);
      } else if (ch == 'f') {
        UPDATE_STATE(s_parse_array_fa);
      } else if (ch == 'n') {
        UPDATE_STATE(s_parse_array_nu);
      }
      break;
    }
    case s_parse_array_find_array_end: {
      if (ch == OB) {
        parser->array_count++;
      } else if (ch == CB) {
        parser->array_count--;
      } else if (ch == QM) {
        UPDATE_STATE(s_parse_array_find_array_end_string_end);
        break;
      }

      if (parser->array_count == 0) {
        UPDATE_STATE(s_parse_array_item_end);
        REEXECUTE();
      }
      break;
    }
    case s_parse_array_find_array_end_string_end: {
      if (ch == QM) {
        UPDATE_STATE(s_parse_array_find_array_end);
      } else if (ch == '\\') {
        p++; // Skip escaped characters.
      }
      break;
    }
    case s_parse_array_item_end: {
      if (callbacks->on_array_value) {
        if (callbacks->on_array_value(parser, parser->array_index,
                                      parser->array_item_mark,
                                      (p - parser->array_item_mark))) {
          SET_ERRNO(ERRNO_CALLBACK_FAILED);
          goto error;
        }
      }
      parser->array_index++;
      UPDATE_STATE(s_parse_array);
      break;
    }
    case s_parse_array_find_object_end: {
      if (ch == OB) {
        parser->object_count++;
      } else if (ch == CB) {
        parser->object_count--;
      } else if (ch == QM) {
        UPDATE_STATE(s_parse_array_find_object_end_string_end);
        break;
      }

      if (parser->object_count == 0) {
        UPDATE_STATE(s_parse_array_item_end);
        REEXECUTE();
      }
      break;
    }
    case s_parse_array_find_object_end_string_end: {
      // TODO combine with s_parse_array_find_array_end_string_end
      if (ch == QM) {
        UPDATE_STATE(s_parse_array_find_object_end);
      } else if (ch == '\\') {
        p++; // Skip escaped characters.
      }
      break;
    }
    default:
      SET_ERRNO(ERRNO_UNKNOWN);
      goto error;
    }
  }

error:
  if (JSON_ERRNO(parser) != ERRNO_OK) {
    SET_ERRNO(ERRNO_UNKNOWN);
  }

  return (p - data);
}
