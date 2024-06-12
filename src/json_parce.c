#include "json_parce.h"
#include "encoding.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define SET_ERR(e)                                                             \
  parser->err = (e);                                                           \
  parser->line = __LINE__;                                                     \
  parser->file = __FILE__;                                                     \
  parser->func = __func__;

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

#define SET_TYPE_VALUE(TYPE, VALUE, LENGTH)                                    \
  GET_TYPE(VALUE[0], TYPE);                                                    \
  if ((TYPE) == STRING) {                                                      \
    /* Remove '"' from each side. */                                           \
    (VALUE)++;                                                                 \
    (LENGTH) -= 2;                                                             \
  }

#define P_OBJECT_CALLBACK(VALUE_LENGTH)                                        \
  if (callbacks->on_object_key_value_pair) {                                   \
    enum JSON_TYPE type = NONE;                                                \
    size_t l_value_length = (VALUE_LENGTH);                                    \
    SET_TYPE_VALUE(type, parser->object_value_mark, l_value_length)            \
    int callback_retval = 0;                                                   \
    char* key = json_parce_string(parser->object_key_mark, parser->object_key_len);  \
    if (!key) {                                                                \
      SET_ERRNO(ERRNO_INVALID_OBJECT_KEY);                                     \
      goto error;                                                              \
    }                                                                          \
    if ((callback_retval = callbacks->on_object_key_value_pair(                \
             parser, key, strlen(key), type,    \
             parser->object_value_mark, l_value_length)) > 0) {                \
      SET_ERRNO(callback_retval == 2 ? ERRNO_CALLBACK_REQUESTED_STOP           \
                                     : ERRNO_CALLBACK_FAILED);                 \
      goto error;                                                              \
    }                                                                          \
  }
#define OBJECT_CALLBACK() P_OBJECT_CALLBACK((p - parser->object_value_mark) + 1)
#define OBJECT_CALLBACK_NOADVANCE()                                            \
  P_OBJECT_CALLBACK((p - parser->object_value_mark))

#define P_ARRAY_CALLBACK(VALUE_LENGTH)                                         \
  if (callbacks->on_array_value) {                                             \
    enum JSON_TYPE type = NONE;                                                \
    size_t l_value_length = (VALUE_LENGTH);                                    \
    SET_TYPE_VALUE(type, parser->array_item_mark, l_value_length)              \
    int callback_retval = 0;                                                   \
    if ((callback_retval = callbacks->on_array_value(                          \
             parser, parser->array_index, type, (parser->array_item_mark),     \
             (l_value_length))) > 0) {                                         \
      SET_ERRNO(callback_retval == 2 ? ERRNO_CALLBACK_REQUESTED_STOP           \
                                     : ERRNO_CALLBACK_FAILED);                 \
      goto error;                                                              \
    }                                                                          \
  }                                                                            \
  parser->array_index++

#define FIND_STRING_END(CHAR, NEW_STATE)                                       \
  if (CHAR == QM) {                                                            \
    UPDATE_STATE(NEW_STATE);                                                   \
  } else if (ch == '\\') {                                                     \
    HANDLE_ESCAPED_CHARACTER(CHAR)                                             \
  }                                                                            \
  CHECK_FOR_UNESCAPED_CHARACTERS(CHAR)                                         \
  break;

#ifdef JSON_PARCE_STRICT_MODE
#define CHECK_FOR_UNESCAPED_CHARACTERS(CHAR)                                   \
  if (SHOULD_ESCAPE(CHAR)) {                                                   \
    SET_ERRNO(ERRNO_INVALID_CHARACTER);                                        \
    goto error;                                                                \
  }
#else
#define CHECK_FOR_UNESCAPED_CHARACTERS(CHAR)
#endif

#ifdef JSON_PARCE_STRICT_MODE
#define HANDLE_ESCAPED_CHARACTER(CHAR)                                         \
  parser->return_state =                                                       \
      p_state; /* save current state to go back after escaped character */     \
  UPDATE_STATE(s_handle_escaped_character);                                    \
  break;
#else
#define HANDLE_ESCAPED_CHARACTER(CHAR)                                         \
  p++;                                                                         \
  break;
#endif

#define FIND_STRING_END_REEXECUTE(CHAR, NEW_STATE)                             \
  if (CHAR == QM) {                                                            \
    UPDATE_STATE(NEW_STATE);                                                   \
    REEXECUTE();                                                               \
  } else if (ch == '\\') {                                                     \
    HANDLE_ESCAPED_CHARACTER(CHAR)                                             \
  }                                                                            \
  CHECK_FOR_UNESCAPED_CHARACTERS(CHAR)                                         \
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
#define SHOULD_ESCAPE(CH) (CH >= '\x00' && CH <= '\x1F')
#define VALID_ESCAPED_CHARACTERS_1(CH)                                         \
  (((CH) == 'n') || ((CH) == 'r') || ((CH) == '\\') || ((CH) == 't') ||        \
   ((CH) == 'u') || ((CH) == '/') || ((CH) == 'b') || ((CH) == 'f') ||         \
   ((CH) == '"'))
#define VALID_ESCAPED_CHARACTERS_2_5(CH)                                       \
  (((CH) >= '0' && (CH) <= '9') || ((CH) >= 'a' && (CH) <= 'f') ||             \
   ((CH) >= 'A' && (CH) <= 'F'))

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
    SET_ERRNO(ERRNO_INVALID_CHARACTER);                                        \
    goto error;                                                                \
  }                                                                            \
  }

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

#ifdef JSON_PARCE_STRICT_MODE

#define RESET_NUMERIC_FLAGS()                                                  \
  parser->f_nonzero = 0;                                                       \
  parser->f_minus = 0;                                                         \
  parser->f_zero = 0

#define RESET_NUMERIC_STATE()                                                  \
  parser->fs_significand = 0;                                                  \
  parser->fs_fraction = 0;                                                     \
  parser->fs_exponent = 0;                                                     \
  RESET_NUMERIC_FLAGS()

#define CHECK_FINAL_NUMERIC_STATE()                                            \
  if ((parser->fs_fraction || parser->fs_exponent) && !parser->f_nonzero) {    \
    SET_ERRNO(ERRNO_INVALID_CHARACTER);                                        \
    goto error;                                                                \
    /*No number, just minus */                                                 \
  } else if (parser->f_minus && !parser->f_nonzero && !parser->f_zero) {       \
    SET_ERRNO(ERRNO_INVALID_CHARACTER);                                        \
    goto error;                                                                \
  }

#endif

enum state {
  s_done = JSON_PARCE_DONE_STATE,
  s_dead = 1,
  s_start,
  s_parse_array_start,
  s_parse_array,
  s_parse_array_string,
  s_parse_array_numeric,
  s_parse_array_nu,
  s_parse_array_nul,
  s_parse_array_null,
  s_parse_array_tr,
  s_parse_array_tru,
  s_parse_array_true,
  s_parse_array_fa,
  s_parse_array_fal,
  s_parse_array_fals,
  s_parse_array_false,
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
  s_parse_object_value_null,
  s_parse_object_value_tr,
  s_parse_object_value_tru,
  s_parse_object_value_true,
  s_parse_object_value_fa,
  s_parse_object_value_fal,
  s_parse_object_value_fals,
  s_parse_object_value_false,
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
  s_handle_escaped_character,
  s_handle_escaped_unicode_character_0,
  s_handle_escaped_unicode_character_1,
  s_handle_escaped_unicode_character_2,
  s_handle_escaped_unicode_character_3,
  s_handle_numeric_significand,
  s_handle_numeric_fraction,
  s_handle_numeric_exponent
};

static void json_depth_init(json_depth *depth) {
  memset(depth, 0, sizeof(*depth));
}

JSON_PARCE_API void json_parce_init(json_parce *parser) {
  memset(parser, 0, sizeof(*parser));
  parser->state = s_start;
}

JSON_PARCE_API void json_parce_free(json_parce *parser) {
  while (parser->current_depth) {
    json_depth *old = parser->current_depth;
    parser->current_depth = parser->current_depth->parent;
    free(old);
  }

  memset(parser, 0, sizeof(*parser));
}

static size_t do_json_parce_execute(json_parce *parser,
                                    json_parce_callbacks *callbacks,
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
      if (ch == OB || ch == OCB) {
        if (deep) {
          assert(parser->current_depth == NULL);
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
      }
#ifdef JSON_PARCE_STRICT_MODE
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
#ifdef JSON_PARCE_STRICT_MODE
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
        REEXECUTE();
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
#ifdef JSON_PARCE_STRICT_MODE
      if (ch != 'u') {
        SET_ERRNO(ERRNO_INVALID_CHARACTER);
        goto error;
      }
      UPDATE_STATE(s_parse_array_nul);
#else
      // If non-strict mode, skip to end.
      p++;
      UPDATE_STATE(s_parse_array_item_end);
#endif
      break;
    }
    case s_parse_array_nul: {
      // Only ran on JSON_PARCE_STRICT_MODE
      if (ch != 'l') {
        SET_ERRNO(ERRNO_INVALID_CHARACTER);
        goto error;
      }
      UPDATE_STATE(s_parse_array_null);
      break;
    }
    case s_parse_array_null: {
      // Only ran on JSON_PARCE_STRICT_MODE
      if (ch != 'l') {
        SET_ERRNO(ERRNO_INVALID_CHARACTER);
        goto error;
      }
      UPDATE_STATE(s_parse_array_item_end);
      REEXECUTE();
    }
    case s_parse_array_tr: {
#ifdef JSON_PARCE_STRICT_MODE
      if (ch != 'r') {
        SET_ERRNO(ERRNO_INVALID_CHARACTER);
        goto error;
      }
      UPDATE_STATE(s_parse_array_tru);
#else
      // If non-strict mode, skip to end.
      p++;
      UPDATE_STATE(s_parse_array_item_end);
#endif
      break;
    }
    case s_parse_array_tru: {
      // Only ran on JSON_PARCE_STRICT_MODE
      if (ch != 'u') {
        SET_ERRNO(ERRNO_INVALID_CHARACTER);
        goto error;
      }
      UPDATE_STATE(s_parse_array_true);
      break;
    }
    case s_parse_array_true: {
      // Only ran on JSON_PARCE_STRICT_MODE
      if (ch != 'e') {
        SET_ERRNO(ERRNO_INVALID_CHARACTER);
        goto error;
      }
      UPDATE_STATE(s_parse_array_item_end);
      REEXECUTE();
    }
    case s_parse_array_fa: {
#ifdef JSON_PARCE_STRICT_MODE
      if (ch != 'a') {
        SET_ERRNO(ERRNO_INVALID_CHARACTER);
        goto error;
      }
      UPDATE_STATE(s_parse_array_fal);
#else
      // If non-strict mode, skip to end.
      p += 2;
      UPDATE_STATE(s_parse_array_item_end);
#endif
      break;
    }
    case s_parse_array_fal: {
      // Only ran on JSON_PARCE_STRICT_MODE
      if (ch != 'l') {
        SET_ERRNO(ERRNO_INVALID_CHARACTER);
        goto error;
      }
      UPDATE_STATE(s_parse_array_fals);
      break;
    }
    case s_parse_array_fals: {
      // Only ran on JSON_PARCE_STRICT_MODE
      if (ch != 's') {
        SET_ERRNO(ERRNO_INVALID_CHARACTER);
        goto error;
      }
      UPDATE_STATE(s_parse_array_item_end);
      break;
    }
    case s_parse_array_false: {
      // Only ran on JSON_PARCE_STRICT_MODE
      if (ch != 'e') {
        SET_ERRNO(ERRNO_INVALID_CHARACTER);
        goto error;
      }
      UPDATE_STATE(s_parse_array_item_end);
      REEXECUTE();
    }
    case s_parse_array_numeric: {
      if (ch == COMMA || ch == CB || IS_WHITESPACE(ch)) {
#ifdef JSON_PARCE_STRICT_MODE
        CHECK_FINAL_NUMERIC_STATE();
        RESET_NUMERIC_STATE();
#endif
        ARRAY_CALLBACK_NOADVANCE();
        UPDATE_STATE(s_parse_array_between_values);
        REEXECUTE();
      }
#ifdef JSON_PARCE_STRICT_MODE
      else {
        parser->return_state = s_parse_array_numeric;
        if (parser->fs_exponent) {
          UPDATE_STATE(s_handle_numeric_exponent);
        } else if (parser->fs_fraction) {
          UPDATE_STATE(s_handle_numeric_fraction);
        } else {
          UPDATE_STATE(s_handle_numeric_significand);
        }
        REEXECUTE();
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
        REEXECUTE();
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
      if (ch == COMMA || ch == CCB || IS_WHITESPACE(ch)) {
#ifdef JSON_PARCE_STRICT_MODE
        CHECK_FINAL_NUMERIC_STATE();
        RESET_NUMERIC_STATE();
#endif
        OBJECT_CALLBACK_NOADVANCE()
        UPDATE_STATE(s_parse_object_between_values);
        REEXECUTE();
      }
#ifdef JSON_PARCE_STRICT_MODE
      else {
        parser->return_state = s_parse_object_value_numeric;
        if (parser->fs_exponent) {
          UPDATE_STATE(s_handle_numeric_exponent);
        } else if (parser->fs_fraction) {
          UPDATE_STATE(s_handle_numeric_fraction);
        } else {
          UPDATE_STATE(s_handle_numeric_significand);
        }
        REEXECUTE();
      }
#endif
      break;
    }
    case s_parse_object_value_nu: {
#ifdef JSON_PARCE_STRICT_MODE
      if (ch != 'u') {
        SET_ERRNO(ERRNO_INVALID_CHARACTER);
        goto error;
      }
      UPDATE_STATE(s_parse_object_value_nul);
#else
      // If non-strict mode, skip to end.
      p++;
      UPDATE_STATE(s_parse_object_value_end);
#endif
      break;
    }
    case s_parse_object_value_nul: {
      // Only ran on JSON_PARCE_STRICT_MODE
      if (ch != 'l') {
        SET_ERRNO(ERRNO_INVALID_CHARACTER);
        goto error;
      }
      UPDATE_STATE(s_parse_object_value_null);
      break;
    }
    case s_parse_object_value_null: {
      // Only ran on JSON_PARCE_STRICT_MODE
      if (ch != 'l') {
        SET_ERRNO(ERRNO_INVALID_CHARACTER);
        goto error;
      }
      UPDATE_STATE(s_parse_object_value_end);
      REEXECUTE();
    }
    case s_parse_object_value_tr: {
#ifdef JSON_PARCE_STRICT_MODE
      if (ch != 'r') {
        SET_ERRNO(ERRNO_INVALID_CHARACTER);
        goto error;
      }
      UPDATE_STATE(s_parse_object_value_tru);
#else
      // If non-strict mode, skip to end.
      p++;
      UPDATE_STATE(s_parse_object_value_end);
#endif
      break;
    }
    case s_parse_object_value_tru: {
      // Only ran on JSON_PARCE_STRICT_MODE
      if (ch != 'u') {
        SET_ERRNO(ERRNO_INVALID_CHARACTER);
        goto error;
      }
      UPDATE_STATE(s_parse_object_value_true);
      break;
    }
    case s_parse_object_value_true: {
      // Only ran on JSON_PARCE_STRICT_MODE
      if (ch != 'e') {
        SET_ERRNO(ERRNO_INVALID_CHARACTER);
        goto error;
      }
      UPDATE_STATE(s_parse_object_value_end);
      REEXECUTE();
    }
    case s_parse_object_value_fa: {
#ifdef JSON_PARCE_STRICT_MODE
      if (ch != 'a') {
        SET_ERRNO(ERRNO_INVALID_CHARACTER);
        goto error;
      }
      UPDATE_STATE(s_parse_object_value_fal);
#else
      // If non-strict mode, skip to end.
      p += 2;
      UPDATE_STATE(s_parse_object_value_end);
#endif
      break;
    }
    case s_parse_object_value_fal: {
      // Only ran on JSON_PARCE_STRICT_MODE
      if (ch != 'l') {
        SET_ERRNO(ERRNO_INVALID_CHARACTER);
        goto error;
      }
      UPDATE_STATE(s_parse_object_value_fals);
      break;
    }
    case s_parse_object_value_fals: {
      // Only ran on JSON_PARCE_STRICT_MODE
      if (ch != 's') {
        SET_ERRNO(ERRNO_INVALID_CHARACTER);
        goto error;
      }
      UPDATE_STATE(s_parse_object_value_false);
      break;
    }
    case s_parse_object_value_false: {
      // Only ran on JSON_PARCE_STRICT_MODE
      if (ch != 'e') {
        SET_ERRNO(ERRNO_INVALID_CHARACTER);
        goto error;
      }
      UPDATE_STATE(s_parse_object_value_end);
      REEXECUTE();
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
#ifdef JSON_PARCE_STRICT_MODE
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
#ifdef JSON_PARCE_STRICT_MODE
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
    case s_handle_escaped_character: {
#ifdef JSON_PARCE_STRICT_MODE
      // Only ran on JSON_PARCE_STRICT_MODE
      if (!VALID_ESCAPED_CHARACTERS_1(ch)) {
        SET_ERRNO(ERRNO_INVALID_CHARACTER);
        goto error;
      }
      if (ch == 'u') {
        UPDATE_STATE(s_handle_escaped_unicode_character_0);
      } else {
        UPDATE_STATE(parser->return_state);
        parser->return_state = s_dead;
      }
#endif
      break;
    }
    case s_handle_escaped_unicode_character_0:
    case s_handle_escaped_unicode_character_1:
    case s_handle_escaped_unicode_character_2:
    case s_handle_escaped_unicode_character_3: {
#ifdef JSON_PARCE_STRICT_MODE
      // Only ran on JSON_PARCE_STRICT_MODE
      if (!VALID_ESCAPED_CHARACTERS_2_5(ch)) {
        SET_ERRNO(ERRNO_INVALID_CHARACTER);
        goto error;
      }
      if (p_state == s_handle_escaped_unicode_character_3) {
        // Go back to original state.
        UPDATE_STATE(parser->return_state);
        parser->return_state = s_dead;
      } else {
        p_state++;
      }
#endif
      break;
    }
    case s_handle_numeric_significand: {
      // Only ran on JSON_PARCE_STRICT_MODE
#ifdef JSON_PARCE_STRICT_MODE
      switch (ch) {
      case '-': {
        if (parser->f_minus || parser->f_zero || parser->f_nonzero) {
          SET_ERRNO(ERRNO_INVALID_CHARACTER);
          goto error;
        }
        parser->f_minus = 1;
        break;
      }
      case '0': {
        parser->f_zero = 1;
        break;
      }
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9': {
        if (parser->f_zero && (!parser->f_nonzero)) {
          // Leading zero.
          SET_ERRNO(ERRNO_INVALID_CHARACTER);
          goto error;
        }
        parser->f_nonzero = 1;
        break;
      }
      case '.': {
        if ((!parser->f_zero) && (!parser->f_nonzero)) {
          // No leading number
          SET_ERRNO(ERRNO_INVALID_CHARACTER);
          goto error;
        }
        parser->fs_fraction = 1;
        RESET_NUMERIC_FLAGS();
        break;
      }
      case 'e':
      case 'E': {
        if ((!parser->f_zero) && (!parser->f_nonzero)) {
          SET_ERRNO(ERRNO_INVALID_CHARACTER);
          goto error;
        }
        parser->fs_exponent = 1;
        RESET_NUMERIC_FLAGS();
        break;
      }
      default: {
        SET_ERRNO(ERRNO_INVALID_CHARACTER);
        goto error;
      }
      }
      UPDATE_STATE(parser->return_state);
      parser->return_state = s_dead;
#endif
      break;
    }
    case s_handle_numeric_fraction: {
#ifdef JSON_PARCE_STRICT_MODE
      switch (ch) {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9': {
        parser->f_nonzero = 1;
        break;
      }
      case 'e':
      case 'E': {
        if (!parser->f_nonzero) {
          SET_ERRNO(ERRNO_INVALID_CHARACTER);
          goto error;
        }
        parser->fs_exponent = 1;
        RESET_NUMERIC_FLAGS();
        break;
      }
      default: {
        SET_ERRNO(ERRNO_INVALID_CHARACTER);
        goto error;
      }
      }
      UPDATE_STATE(parser->return_state);
      parser->return_state = s_dead;
#endif
      break;
    }
    case s_handle_numeric_exponent: {
#ifdef JSON_PARCE_STRICT_MODE
      switch (ch) {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9': {
        parser->f_nonzero = 1;
        break;
      }
      case '+':
      case '-': {
        if (parser->f_minus || parser->f_nonzero) {
          SET_ERRNO(ERRNO_INVALID_CHARACTER);
          goto error;
        }
        parser->f_minus = 1;
        break;
      }
      default: {
        SET_ERRNO(ERRNO_INVALID_CHARACTER);
        goto error;
      }
      }
      UPDATE_STATE(parser->return_state);
      parser->return_state = s_dead;
#endif
      break;
    }
    }
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

  // fprintf(stderr, "%s(%d) - %s\n", parser->file, parser->line,
  //         json_errno_messages[parser->err]);
  // if (parser->err == ERRNO_INVALID_CHARACTER) {
  //   fprintf(stderr, "Invalid character: \"%c\"\n", *p);
  // }

  RETURN(p - data);
}

JSON_PARCE_API size_t json_parce_execute(json_parce *parser,
                                         json_parce_callbacks *callbacks,
                                         const char *data, size_t len) {
  return do_json_parce_execute(parser, callbacks, data, len, 0);
}

JSON_PARCE_API size_t json_deep_parce_execute(json_parce *parser,
                                              json_parce_callbacks *callbacks,
                                              const char *data, size_t len) {
  return do_json_parce_execute(parser, callbacks, data, len, 1);
}

JSON_PARCE_API size_t json_parce_execute_utf16(json_parce *parser,
                                               json_parce_callbacks *callbacks,
                                               const char16_t *data,
                                               size_t len) {
  char *content = NULL;
  size_t content_len = 0;

  int encoding = check_json_byte_encoding((uint8_t *)data);

  if (c16strtomb(data, len, encoding, &content, &content_len) < 0) {
    SET_ERR(ERRNO_INVALID_ENCODING);
    return 0;
  }

  size_t retval = json_parce_execute(parser, callbacks, content, content_len);

  free(content);
  return retval;
}

JSON_PARCE_API size_t json_parce_execute_utf32(json_parce *parser,
                                               json_parce_callbacks *callbacks,
                                               const char32_t *data,
                                               size_t len) {
  char *content = NULL;
  size_t content_len = 0;

  int encoding = check_json_byte_encoding((uint8_t *)data);

  if (c32strtomb(data, len, encoding, &content, &content_len) < 0) {
    SET_ERR(ERRNO_INVALID_ENCODING);
    return 0;
  }

  size_t retval = json_parce_execute(parser, callbacks, content, content_len);

  free(content);
  return retval;
}

static size_t do_json_parce_execute_file(json_parce *parser,
                                         json_parce_callbacks *callbacks,
                                         const char *file, int deep) {

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

  size_t retval =
      do_json_parce_execute(parser, callbacks, content, content_len, deep);

  free(content);

  return retval;
}

JSON_PARCE_API size_t json_parce_execute_file(json_parce *parser,
                                              json_parce_callbacks *callbacks,
                                              const char *file) {
  return do_json_parce_execute_file(parser, callbacks, file, 0);
}

JSON_PARCE_API size_t json_deep_parce_execute_file(
    json_parce *parser, json_parce_callbacks *callbacks, const char *file) {
  return do_json_parce_execute_file(parser, callbacks, file, 1);
}

static size_t decode_string(char *str, size_t len) {
  char *p = str;

  for (size_t i = 0; i < len; i++) {
    if (str[i] == '\\') {
      char next = str[i + 1];
      switch (str[i + 1]) {
      case '"': {
        *p = next;
        i++;
        break;
      }
      default: {
        *p = str[i];
      }
      }
    } else {
      *p = str[i];
    }
    p++;
  }
  *p = '\0';

  return strlen(str);
}

JSON_PARCE_API char *json_parce_string(const char *str, size_t len) {
  char *ret = (char *)malloc((len + 1) * sizeof(char));
  memcpy(ret, str, len);
  ret[len] = '\0';

  if (memchr(ret, '\\', len) != NULL) {
    // need to decode characters.
    len = decode_string(ret, len);

    char* new_ret = NULL;
    int status = process_unicode_escape_string(ret, &new_ret);
    if (status == 0) {
      free(ret);
      ret = new_ret;
    } else {
      return NULL;
    }
  }

  return ret;
}

JSON_PARCE_API int json_parce_bool(const char *str) {
  return (str && str[0] && (str[0] == 't'));
}

JSON_PARCE_API int json_parce_real(const char *str, size_t len,
                                   json_parce_real_t *ret) {
  char lstr[JSON_PARCE_REAL_MAX_DIG + 1] = {0};

  if (len > JSON_PARCE_REAL_MAX_DIG) {
    return ERRNO_OUT_OF_RANGE;
  }

  strncpy(lstr, str, len);
  int status = sscanf(lstr, "%lf", ret);
  if (!status) {
    return ERRNO_OUT_OF_RANGE;
  }

  return 0;
}

JSON_PARCE_API int json_parce_int(const char *str, size_t len,
                                  json_parce_int_t *ret) {
  char lstr[JSON_PARCE_INT_MAX_DIG + 1] = {0};

  if (len > JSON_PARCE_INT_MAX_DIG) {
    return ERRNO_OUT_OF_RANGE;
  }

  if (memchr(str, '.', len) != NULL) {
    // sscanf will convert a double into an int, so check for a decimal in the
    // number string.
    // TODO: What about 1.00000? Do we still want thaat to be a double?
    return ERRNO_INVALID_CHARACTER;
  }

  strncpy(lstr, str, len);
  int status = sscanf(lstr, "%lld", ret);
  if (status != 1) {
    return ERRNO_OUT_OF_RANGE;
  }

  return 0;
}
