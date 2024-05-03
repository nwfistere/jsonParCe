#include "c_json_parser.h"
#include "encoding.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// TODO: if not __func__ just set it to unknown or something.
#ifdef __func__
#define SET_ERR(e)                                                             \
  parser->err = (e);                                                           \
  parser->line = __LINE__;                                                     \
  parser->file = __FILE__;                                                     \
  parser->func = __func__;
#else
#define SET_ERR(e)                                                             \
  parser->err = (e);                                                           \
  parser->line = __LINE__;                                                     \
  parser->file = __FILE__;
#endif // #ifdef __func__

#define SET_ERRNO(e)                                                           \
  do {                                                                         \
    parser->nread = nread;                                                     \
    SET_ERR(e);                                                                \
  } while (0)
#define RETURN(V)                                                              \
  do {                                                                         \
    parser->nread = (size_t)nread;                                             \
    parser->state = (unsigned int)p_state;                                     \
    return (V);                                                                \
  } while (0)

#define JSON_ERRNO(parser) ((enum json_errno)(parser->err))
#define MARK_ARRAY_ITEM_START(P) parser->array_item_mark = (P)
#define MARK_OBJECT_KEY_START(P) parser->object_key_mark = (P)
#define MARK_OBJECT_KEY_LENGTH(P)                                              \
  parser->object_key_len = (size_t)((P)-parser->object_key_mark)
#define MARK_OBJECT_VALUE_START(P) parser->object_value_mark = (P)

#define P_OBJECT_CALLBACK(VALUE_LENGTH)                                        \
  if (callbacks->on_object_key_value_pair) {                                   \
    if (callbacks->on_object_key_value_pair(                                   \
            parser, parser->object_key_mark, parser->object_key_len,           \
            parser->object_value_mark, (size_t)(VALUE_LENGTH))) {              \
      SET_ERRNO(ERRNO_CALLBACK_FAILED);                                        \
      goto error;                                                              \
    }                                                                          \
  }
#define OBJECT_CALLBACK() P_OBJECT_CALLBACK((p - parser->object_value_mark) + 1)
#define OBJECT_CALLBACK_NOADVANCE()                                            \
  P_OBJECT_CALLBACK((p - parser->object_value_mark))

#define P_ARRAY_CALLBACK(VALUE_LENGTH)                                         \
  if (callbacks->on_array_value) {                                             \
    if (callbacks->on_array_value(parser, parser->array_index,                 \
                                  parser->array_item_mark,                     \
                                  (size_t)(VALUE_LENGTH))) {                   \
      SET_ERRNO(ERRNO_CALLBACK_FAILED);                                        \
      goto error;                                                              \
    }                                                                          \
  }                                                                            \
  parser->array_index++

#define FIND_STRING_END(CHAR, NEW_STATE)                                       \
  if (CHAR == QM) {                                                            \
    UPDATE_STATE(NEW_STATE);                                                   \
  } else if (ch == '\\') {                                                     \
    p++;                                                                       \
  }                                                                            \
  break;

#define FIND_STRING_END_REEXECUTE(CHAR, NEW_STATE)                             \
  if (CHAR == QM) {                                                            \
    UPDATE_STATE(NEW_STATE);                                                   \
    REEXECUTE();                                                               \
  } else if (ch == '\\') {                                                     \
    p++;                                                                       \
  }                                                                            \
  break;

#define ARRAY_CALLBACK() P_ARRAY_CALLBACK((p - parser->array_item_mark) + 1)
#define ARRAY_CALLBACK_NOADVANCE()                                             \
  P_ARRAY_CALLBACK((p - parser->array_item_mark))

#define OB '['
#define CB ']'
#define OCB '{'
#define CCB '}'
#define CR '\r'
#define LF '\n'
#define QM '"'
#define COMMA ','
#define COLON ':'

#define IS_NEWLINE(c) (c) == LF || (c) == CR
#define IS_WHITESPACE(c) ((c) == ' ' || (c) == '\t' || (c) == LF || (c) == CR)
#define IS_NUMERIC(c)                                                          \
  (((c) >= '0' && (c) <= '9') || (c) == '-' || (c) == '.' || (c) == '+' ||     \
   (c) == 'e' || (c) == 'E')
#define IS_BOOL(c) ((c) == 't' || (c) == 'f')
#define IS_ARRAY_TOKEN(c)                                                      \
  ((c) == QM || (c) == OB || (c) == OCB || IS_BOOL(c) || c == 'n' ||           \
   IS_NUMERIC(c))
#define UPDATE_STATE(V) p_state = (enum state)(V)
#define REEXECUTE() goto reexecute

#define GET_TYPE(C, T)                                                         \
  switch (C) {                                                                 \
  case QM: {                                                                   \
    (T) = STRING;                                                              \
    break;                                                                     \
  }                                                                            \
  case OB: {                                                                   \
    (T) = ARRAY;                                                               \
    break;                                                                     \
  }                                                                            \
  case OCB: {                                                                  \
    (T) = OBJECT;                                                              \
    break;                                                                     \
  }                                                                            \
  case 'n': {                                                                  \
    (T) = NULL_TYPE;                                                           \
    break;                                                                     \
  }                                                                            \
  case 't':                                                                    \
  case 'f': {                                                                  \
    (T) = BOOL_TYPE;                                                           \
    break;                                                                     \
  }                                                                            \
  case '0':                                                                    \
  case '1':                                                                    \
  case '2':                                                                    \
  case '3':                                                                    \
  case '4':                                                                    \
  case '5':                                                                    \
  case '6':                                                                    \
  case '7':                                                                    \
  case '8':                                                                    \
  case '9':                                                                    \
  case '-': {                                                                  \
    (T) = NUMBER;                                                              \
    break;                                                                     \
  }                                                                            \
  default: {                                                                   \
    (T) = NONE;                                                                \
    break;                                                                     \
  }                                                                            \
  }

#define UPDATE_PARSER(LP, RP)                                                  \
  do {                                                                         \
    /* Copy over everything but data. */                                       \
    (LP)->state = (RP)->state;                                                 \
    (LP)->err = (RP)->err;                                                     \
    (LP)->nread = (RP)->nread;                                                 \
    (LP)->array_item_mark = (RP)->array_item_mark;                             \
    (LP)->array_index = (RP)->array_index;                                     \
    (LP)->array_count = (RP)->array_count;                                     \
    (LP)->object_count = (RP)->object_count;                                   \
    (LP)->object_key_mark = (RP)->object_key_mark;                             \
    (LP)->object_key_len = (RP)->object_key_len;                               \
    (LP)->object_value_mark = (RP)->object_value_mark;                         \
    (LP)->current_depth = (RP)->current_depth;                                 \
    (LP)->max_depth = (RP)->max_depth;                                         \
  } while (0)

#define INCREASE_DEPTH(NEW_STATE, MAX_DEPTH_STATE)                             \
  if ((parser->max_depth > 0) &&                                               \
      (parser->current_depth->depth == parser->max_depth)) {                   \
    /* hit max depth, stop here. */                                            \
    UPDATE_STATE(MAX_DEPTH_STATE);                                             \
    REEXECUTE();                                                               \
  } else {                                                                     \
    json_depth *new_depth = (json_depth *)malloc(sizeof(json_depth));          \
    json_depth_init(new_depth);                                                \
    new_depth->depth = parser->current_depth->depth + 1;                       \
    new_depth->parent = parser->current_depth;                                 \
    new_depth->key = parser->object_key_mark;                                  \
    new_depth->key_len = parser->object_key_len;                               \
    new_depth->array_index = parser->array_index;                              \
    if (ch == OCB) {                                                           \
      new_depth->type = 1;                                                     \
    } else {                                                                   \
      /* leave type as zero for array. */                                      \
    }                                                                          \
    parser->object_key_mark = NULL;                                            \
    parser->object_key_len = 0;                                                \
    parser->array_index = 0;                                                   \
    parser->current_depth = new_depth;                                         \
    UPDATE_STATE(NEW_STATE);                                                   \
  }                                                                            \
  break;

// Will need the following states:
// - Parent type (Array or Object)
// - Current depth
#define DECREASE_DEPTH()                                                       \
  if (parser->current_depth->depth == 0) {                                     \
    UPDATE_STATE(s_done);                                                      \
  } else {                                                                     \
    json_depth *old_depth = parser->current_depth;                             \
    parser->object_key_mark = old_depth->key;                                  \
    parser->object_key_len = old_depth->key_len;                               \
    parser->array_index = old_depth->array_index;                              \
    parser->current_depth = parser->current_depth->parent;                     \
    free(old_depth);                                                           \
    if (parser->current_depth->type) {                                         \
      UPDATE_STATE(s_parse_object_between_values);                             \
    } else {                                                                   \
      /* since we're not calling the callback, we need to manually increment   \
       * the array_index. */                                                   \
      parser->array_index++;                                                   \
      UPDATE_STATE(s_parse_array_between_values);                              \
    }                                                                          \
  }

#define VALIDATE_CH(EXPECTED)                                                  \
  if ((ch) != (EXPECTED)) {                                                    \
    SET_ERRNO(ERRNO_INVALID_CHARACTER);                                        \
    goto error;                                                                \
  }

enum state {
  s_done = C_JSON_PARSER_DONE_STATE,
  s_dead = 1,
  s_start,
  s_parse_array_start,
  s_parse_array,
  s_parse_array_string,
  s_parse_array_numeric,
  s_parse_array_nu,
  s_parse_array_nul,
  s_parse_array_tr,
  s_parse_array_tru,
  s_parse_array_fa,
  s_parse_array_fal,
  s_parse_array_fals,
  s_parse_array_find_array_end,
  s_parse_array_find_array_end_string_end,
  s_parse_array_find_object_end_string_end,
  s_parse_array_item_end,
  s_parse_array_find_object_end,
  s_parse_object_start,
  s_parse_object,
  s_parse_object_parse_key,
  s_parse_object_parse_key_end,
  s_parse_object_colon,
  s_parse_object_value,
  s_parse_object_value_string,
  s_parse_object_value_numeric,
  s_parse_object_value_nu,
  s_parse_object_value_nul,
  s_parse_object_value_tr,
  s_parse_object_value_tru,
  s_parse_object_value_fa,
  s_parse_object_value_fal,
  s_parse_object_value_fals,
  s_parse_object_value_find_array_end,
  s_parse_object_value_find_array_end_string_end,
  s_parse_object_value_find_object_end,
  s_parse_object_value_find_object_end_string_end,
  s_parse_object_value_end,
  s_parse_array_between_values,
  s_parse_object_between_values,
  s_parse_BOM_EF,
  s_parse_BOM_BB,
  s_parse_BOM_BF,
};

typedef struct json_parser_typed {
  json_parser *parser;
  json_parser_callbacks_typed *callbacks;
} json_parser_typed;

static void json_depth_init(json_depth *depth) {
  memset(depth, 0, sizeof(*depth));
}

C_JSON_PARSER_API void json_parser_init(json_parser *parser) {
  memset(parser, 0, sizeof(*parser));
  parser->state = s_start;
}

static size_t do_json_parser_execute(json_parser *parser,
                                     json_parser_callbacks *callbacks,
                                     const char *data, size_t len,
                                     const int deep) {
  const char *p = data;
  enum state p_state = (enum state)parser->state;
  size_t nread = parser->nread;
  char ch;

  for (p = data; p < data + len; p++) {
    ch = *p;
  reexecute:
    switch (p_state) {
    case s_dead: {
      SET_ERRNO(ERRNO_INVALID_STATE);
      goto error;
    }
    case s_start: {
      if (IS_WHITESPACE(ch)) {
        break;
      }
      if (deep) {
        parser->current_depth = (json_depth *)malloc(sizeof(json_depth));
        json_depth_init(parser->current_depth);
      }
      if (ch == OB) {
        UPDATE_STATE(s_parse_array_start);
      } else if (ch == OCB) {
        if (deep) {
          parser->current_depth->type = 1;
        }
        UPDATE_STATE(s_parse_object_start);
      }
#ifdef C_JSON_PARSER_STRICT_MODE
      else {
        UPDATE_STATE(s_parse_BOM_EF);
        REEXECUTE();
      }
#endif
      break;
    }
    case s_parse_array_start: {
      // Initial case because we normally don't accept a ']'
      // in s_parse_array, but if the array is empty, it's allowed.
      if (IS_WHITESPACE(ch)) {
        break;
      } else if (ch == CB) {
        if (deep) {
          DECREASE_DEPTH();
        } else {
          UPDATE_STATE(s_done);
        }
        break;
      }
#ifdef C_JSON_PARSER_STRICT_MODE
      else if (!IS_ARRAY_TOKEN(ch)) {
        SET_ERRNO(ERRNO_INVALID_CHARACTER);
        goto error;
      }
#endif
      else {
        UPDATE_STATE(s_parse_array);
        REEXECUTE();
      }
    }
    case s_parse_array: {
      if (IS_WHITESPACE(ch)) {
        break;
      }
      MARK_ARRAY_ITEM_START(p);
      if (ch == OB) {
        if (deep) {
          INCREASE_DEPTH(s_parse_array_start, s_parse_array_find_array_end);
        } else {
          UPDATE_STATE(s_parse_array_find_array_end);
          REEXECUTE();
        }
      } else if (ch == OCB) {
        if (deep) {
          INCREASE_DEPTH(s_parse_object_start, s_parse_array_find_object_end);
        } else {
          UPDATE_STATE(s_parse_array_find_object_end);
          REEXECUTE();
        }
      } else if (ch == QM) {
        UPDATE_STATE(s_parse_array_string);
      } else if (ch == 't') {
        UPDATE_STATE(s_parse_array_tr);
      } else if (ch == 'f') {
        UPDATE_STATE(s_parse_array_fa);
      } else if (ch == 'n') {
        UPDATE_STATE(s_parse_array_nu);
      } else if (IS_NUMERIC(ch)) {
        UPDATE_STATE(s_parse_array_numeric);
      } else {
        // TODO: Only do during strict?
        SET_ERRNO(ERRNO_INVALID_CHARACTER);
        goto error;
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
      FIND_STRING_END(ch, s_parse_array_find_array_end);
    }
    case s_parse_array_item_end: {
      ARRAY_CALLBACK();
      UPDATE_STATE(s_parse_array_between_values);
      break;
    }
    case s_parse_array_find_object_end: {
      if (ch == OCB) {
        parser->object_count++;
      } else if (ch == CCB) {
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
      FIND_STRING_END(ch, s_parse_array_find_object_end);
    }
    case s_parse_array_string: {
      FIND_STRING_END_REEXECUTE(ch, s_parse_array_item_end);
    }
    case s_parse_array_nu: {
      UPDATE_STATE(s_parse_array_nul);
      break;
    }
    case s_parse_array_nul: {
      UPDATE_STATE(s_parse_array_item_end);
      break;
    }
    case s_parse_array_tr: {
      UPDATE_STATE(s_parse_array_tru);
      break;
    }
    case s_parse_array_tru: {
      UPDATE_STATE(s_parse_array_item_end);
      break;
    }
    case s_parse_array_fa: {
      UPDATE_STATE(s_parse_array_fal);
      break;
    }
    case s_parse_array_fal: {
      UPDATE_STATE(s_parse_array_fals);
      break;
    }
    case s_parse_array_fals: {
      UPDATE_STATE(s_parse_array_item_end);
      break;
    }
    case s_parse_array_numeric: {
      if (ch == COMMA || ch == CB || IS_WHITESPACE(ch)) {
        ARRAY_CALLBACK_NOADVANCE();
        UPDATE_STATE(s_parse_array_between_values);
        REEXECUTE();
      }

#ifdef C_JSON_PARSER_STRICT_MODE
      if (!IS_NUMERIC(ch)) {
        SET_ERRNO(ERRNO_INVALID_CHARACTER);
        goto error;
      }
#endif
      break;
    }
    case s_parse_object_start: {
      // Initial case because we normally don't accept a '}'
      // in s_parse_array, but if the array is empty, it's allowed.
      if (IS_WHITESPACE(ch)) {
        break;
      } else if (ch == CCB) {
        if (deep) {
          DECREASE_DEPTH();
        } else {
          UPDATE_STATE(s_done);
        }
        break;
      } else {
        UPDATE_STATE(s_parse_object);
        REEXECUTE();
      }
    }
    case s_parse_object: {
      if (IS_WHITESPACE(ch)) {
        break;
      }
      if (ch == QM) {
        MARK_OBJECT_KEY_START(p + 1);
        UPDATE_STATE(s_parse_object_parse_key);
      } else {
        SET_ERRNO(ERRNO_INVALID_CHARACTER);
        goto error;
      }
      break;
    }
    case s_parse_object_parse_key: {
      FIND_STRING_END_REEXECUTE(ch, s_parse_object_parse_key_end);
    }
    case s_parse_object_parse_key_end: {
      MARK_OBJECT_KEY_LENGTH(p);
      UPDATE_STATE(s_parse_object_colon);
      break;
    }
    case s_parse_object_colon: {
      if (IS_WHITESPACE(ch)) {
        break;
      } else if (ch == COLON) {
        UPDATE_STATE(s_parse_object_value);
      }
      break;
    }
    case s_parse_object_value: {
      if (IS_WHITESPACE(ch)) {
        break;
      }
      MARK_OBJECT_VALUE_START(p);
      if (ch == QM) {
        UPDATE_STATE(s_parse_object_value_string);
      } else if (ch == OCB) {
        if (deep) {
          INCREASE_DEPTH(s_parse_object_start,
                         s_parse_object_value_find_object_end);
        } else {
          UPDATE_STATE(s_parse_object_value_find_object_end);
          REEXECUTE();
        }
      } else if (ch == OB) {
        if (deep) {
          INCREASE_DEPTH(s_parse_array_start,
                         s_parse_object_value_find_array_end);
        } else {
          UPDATE_STATE(s_parse_object_value_find_array_end);
          REEXECUTE();
        }
      } else if (ch == 't') {
        UPDATE_STATE(s_parse_object_value_tr);
      } else if (ch == 'f') {
        UPDATE_STATE(s_parse_object_value_fa);
      } else if (ch == 'n') {
        UPDATE_STATE(s_parse_object_value_nu);
      } else if (IS_NUMERIC(ch)) {
        UPDATE_STATE(s_parse_object_value_numeric);
      } else {
        SET_ERRNO(ERRNO_INVALID_CHARACTER);
        goto error;
      }
      break;
    }
    case s_parse_object_value_string: {
      FIND_STRING_END_REEXECUTE(ch, s_parse_object_value_end);
    }
    case s_parse_object_value_numeric: {
      if (ch == COMMA || ch == CCB) {
        OBJECT_CALLBACK_NOADVANCE()
        UPDATE_STATE(s_parse_object_between_values);
        REEXECUTE();
      }
      break;
    }
    case s_parse_object_value_nu: {
      UPDATE_STATE(s_parse_object_value_nul);
      break;
    }
    case s_parse_object_value_nul: {
      UPDATE_STATE(s_parse_object_value_end);
      break;
    }
    case s_parse_object_value_tr: {
      UPDATE_STATE(s_parse_object_value_tru);
      break;
    }
    case s_parse_object_value_tru: {
      UPDATE_STATE(s_parse_object_value_end);
      break;
    }
    case s_parse_object_value_fa: {
      UPDATE_STATE(s_parse_object_value_fal);
      break;
    }
    case s_parse_object_value_fal: {
      UPDATE_STATE(s_parse_object_value_fals);
      break;
    }
    case s_parse_object_value_fals: {
      UPDATE_STATE(s_parse_object_value_end);
      break;
    }
    case s_parse_object_value_find_array_end: {
      if (ch == OB) {
        parser->array_count++;
      } else if (ch == CB) {
        parser->array_count--;
      } else if (ch == QM) {
        UPDATE_STATE(s_parse_object_value_find_array_end_string_end);
        break;
      }

      if (parser->array_count == 0) {
        UPDATE_STATE(s_parse_object_value_end);
        REEXECUTE();
      }
      break;
    }
    case s_parse_object_value_find_array_end_string_end: {
      FIND_STRING_END(ch, s_parse_object_value_find_array_end);
    }
    case s_parse_object_value_find_object_end: {
      if (ch == OCB) {
        parser->object_count++;
      } else if (ch == CCB) {
        parser->object_count--;
      } else if (ch == QM) {
        UPDATE_STATE(s_parse_object_value_find_object_end_string_end);
        break;
      }

      if (parser->object_count == 0) {
        UPDATE_STATE(s_parse_object_value_end);
        REEXECUTE();
      }
      break;
    }
    case s_parse_object_value_find_object_end_string_end: {
      FIND_STRING_END(ch, s_parse_object_value_find_object_end);
    }
    case s_parse_object_value_end: {
      OBJECT_CALLBACK()
      UPDATE_STATE(s_parse_object_between_values);
      break;
    }
    case s_parse_array_between_values: {
      if (ch == COMMA) {
        UPDATE_STATE(s_parse_array);
      } else if (ch == CB) {
        if (deep) {
          DECREASE_DEPTH();
        } else {
          UPDATE_STATE(s_done);
        }
      }
#ifdef C_JSON_PARSER_STRICT_MODE
      else if (!IS_WHITESPACE(ch)) {
        SET_ERRNO(ERRNO_INVALID_CHARACTER);
        goto error;
      }
#endif
      break;
    }
    case s_parse_object_between_values: {
      if (ch == COMMA) {
        UPDATE_STATE(s_parse_object);
      } else if (ch == CCB) {
        if (deep) {
          DECREASE_DEPTH();
        } else {
          UPDATE_STATE(s_done);
        }
      }
#ifdef C_JSON_PARSER_STRICT_MODE
      else if (!IS_WHITESPACE(ch)) {
        SET_ERRNO(ERRNO_INVALID_CHARACTER);
        goto error;
      }
#endif
      break;
    }
    case s_done: {
      if (IS_WHITESPACE(ch)) {
        // Get to the end.
        break;
      }
      if (p != (data + len)) {
        // String is longer than expected, set error.
        SET_ERRNO(ERRNO_INVALID_CHARACTER);
        goto error;
      }
      break;
    }
    case s_parse_BOM_EF: {
      VALIDATE_CH('\xEF')
      UPDATE_STATE(s_parse_BOM_BB);
      break;
    }
    case s_parse_BOM_BB: {
      VALIDATE_CH('\xBB')
      UPDATE_STATE(s_parse_BOM_BF);
      break;
    }
    case s_parse_BOM_BF: {
      VALIDATE_CH('\xBF')
      UPDATE_STATE(s_start);
      break;
    }
    }
  }

  if (deep) {
    free(parser->current_depth);
  }

  if (p_state != s_done) {
    // This may or may-not be an actual error.
    // The user may be sending in a partial buffer.
    SET_ERRNO(ERRNO_INCOMPLETE_DATA);
    goto error;
  }

  RETURN(len);

error:
  if (JSON_ERRNO(parser) == ERRNO_OK) {
    SET_ERRNO(ERRNO_UNKNOWN);
  }

  RETURN(p - data);
}

C_JSON_PARSER_API size_t json_parser_execute(json_parser *parser,
                                             json_parser_callbacks *callbacks,
                                             const char *data, size_t len) {
  return do_json_parser_execute(parser, callbacks, data, len, 0);
}

C_JSON_PARSER_API size_t
json_deep_parser_execute(json_parser *parser, json_parser_callbacks *callbacks,
                         const char *data, size_t len) {
  return do_json_parser_execute(parser, callbacks, data, len, 1);
}

C_JSON_PARSER_API size_t
json_parser_execute_utf16(json_parser *parser, json_parser_callbacks *callbacks,
                          const char16_t *data, size_t len) {
  char *content = NULL;
  size_t content_len = 0;

  int encoding = check_json_byte_encoding((uint8_t *)data);

  if (c16strtomb(data, len, encoding, &content, &content_len) < 0) {
    SET_ERR(ERRNO_INVALID_ENCODING);
    return 0;
  }

  size_t retval = json_parser_execute(parser, callbacks, content, content_len);

  free(content);
  return retval;
}

C_JSON_PARSER_API size_t
json_parser_execute_utf32(json_parser *parser, json_parser_callbacks *callbacks,
                          const char32_t *data, size_t len) {
  char *content = NULL;
  size_t content_len = 0;

  int encoding = check_json_byte_encoding((uint8_t *)data);

  if (c32strtomb(data, len, encoding, &content, &content_len) < 0) {
    SET_ERR(ERRNO_INVALID_ENCODING);
    return 0;
  }

  size_t retval = json_parser_execute(parser, callbacks, content, content_len);

  free(content);
  return retval;
}

C_JSON_PARSER_API size_t json_parser_execute_file(
    json_parser *parser, json_parser_callbacks *callbacks, const char *file) {

  int encoding = UNKNOWN;
  size_t file_size = 0;
  FILE *fp;
  if (get_file_info(file, &fp, &encoding, &file_size) != 0) {
    SET_ERR(ERRNO_FILE_OPEN_FAILURE);
    return 0;
  }

  if (encoding == 0) {
    SET_ERR(ERRNO_INVALID_ENCODING);
    fclose(fp);
    return 0;
  }

  char *content = NULL;
  size_t content_len = 0;
  if (IS_UTF16(encoding)) {
    char16_t *chars = (char16_t *)malloc((file_size + 1) * sizeof(char16_t));
    char16_t *p = &chars[0];
    size_t nread = 0;

    while ((nread = fread(p, sizeof(char16_t), &chars[file_size] - p, fp)) >
           0) {
      p += nread;
    }
    *p = u'\0';

    if (c16strtomb(chars, strlen16(chars), encoding, &content, &content_len) <
        0) {
      SET_ERR(ERRNO_INVALID_ENCODING);
      fclose(fp);
      free(chars);
      return 0;
    }

    free(chars);
  } else if (IS_UTF32(encoding)) {
    char32_t *chars = (char32_t *)malloc((file_size + 1) * sizeof(char32_t));
    char32_t *p = &chars[0];
    size_t nread = 0;

    while ((nread = fread(p, sizeof(char32_t), &chars[file_size] - p, fp)) >
           0) {
      p += nread;
    }
    *p = U'\0';

    if (c32strtomb(chars, strlen32(chars), encoding, &content, &content_len) <
        0) {
      SET_ERR(ERRNO_INVALID_ENCODING);
      fclose(fp);
      free(chars);
      return 0;
    }
    free(chars);
  } else {
    content = (char *)malloc((file_size + 1) * sizeof(char));
    char *p = &content[0];
    size_t nread = 0;

    while ((nread = fread(p, 1, &content[file_size] - p, fp)) > 0) {
      p += nread;
    }
    content_len = p - &content[0];
    content[content_len] = '\0';
  }

  if (ferror(fp)) {
    SET_ERR(ERRNO_FILE_OPEN_FAILURE);
    fclose(fp);
    free(content);
    return 0;
  }
  fclose(fp);

  size_t retval = json_parser_execute(parser, callbacks, content, content_len);

  free(content);

  return retval;
}

static int json_parser_typed_execute_json_object_cb(json_parser *parser,
                                                    const char *key,
                                                    size_t key_len,
                                                    const char *value,
                                                    size_t value_length) {
  json_parser_typed *_data = parser->data;
  if (_data->callbacks->on_object_key_value_pair) {
    const char ch = *value;
    enum JSON_TYPE type;
    GET_TYPE(ch, type)
    if (type == STRING) {
      // Remove '"' from each side.
      value++;
      value_length -= 2;
    }
    UPDATE_PARSER(_data->parser, parser);
    return _data->callbacks->on_object_key_value_pair(
        _data->parser, key, key_len, type, value, value_length);
  }
  return 0;
}

static int json_parser_typed_execute_json_array_cb(json_parser *parser,
                                                   unsigned int index,
                                                   const char *value,
                                                   size_t value_length) {
  json_parser_typed *_data = parser->data;
  if (_data->callbacks->on_array_value) {
    const char ch = *value;
    enum JSON_TYPE type;
    GET_TYPE(ch, type)
    if (type == STRING) {
      // Remove '"' from each side.
      value++;
      value_length -= 2;
    }
    UPDATE_PARSER(_data->parser, parser);
    return _data->callbacks->on_array_value(_data->parser, index, type, value,
                                            value_length);
  }
  return 0;
}

C_JSON_PARSER_API size_t json_parser_typed_execute(
    json_parser *parser, json_parser_callbacks_typed *callbacks,
    const char *data, size_t len) {
  size_t retval;
  json_parser_callbacks cbs = {
      .on_object_key_value_pair = json_parser_typed_execute_json_object_cb,
      .on_array_value = json_parser_typed_execute_json_array_cb};

  json_parser_typed _data = {.parser = parser, .callbacks = callbacks};

  json_parser _parser = *parser;
  _parser.data = &_data;

  retval = json_parser_execute(&_parser, &cbs, data, len);
  UPDATE_PARSER(parser, &_parser);
  return retval;
}

C_JSON_PARSER_API size_t json_deep_parser_typed_execute(
    json_parser *parser, json_parser_callbacks_typed *callbacks,
    const char *data, size_t len) {
  size_t retval;
  json_parser_callbacks cbs = {
      .on_object_key_value_pair = json_parser_typed_execute_json_object_cb,
      .on_array_value = json_parser_typed_execute_json_array_cb};

  json_parser_typed _data = {.parser = parser, .callbacks = callbacks};

  json_parser _parser = *parser;
  _parser.data = &_data;

  retval = json_deep_parser_execute(&_parser, &cbs, data, len);
  UPDATE_PARSER(parser, &_parser);
  return retval;
}
