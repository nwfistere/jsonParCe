#include "parser.h"
#include <string.h>

#define SET_ERRNO(e)                                                           \
  do {                                                                         \
    parser->nread = nread;                                                     \
    parser->err = (e);                                                         \
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
#define IS_WHITESPACE(c) (c) == ' ' || (c) == '\t' || (c) == LF || (c) == CR
#define IS_NUMERIC(c)                                                          \
  ((c) >= '0' && (c) <= '9') || (c) == '-' || (c) == '.' || (c) == '+' ||      \
      (c) == 'e' || (c) == 'E'
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
  } while (0)

enum state {
  s_dead = 1,
  s_start,
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
  s_done
};

typedef struct json_parser_typed {
  json_parser *parser;
  json_parser_callbacks_typed *callbacks;
} json_parser_typed;

void json_parser_init(json_parser *parser) {
  memset(parser, 0, sizeof(*parser));
  parser->state = s_start;
}

size_t json_parser_execute(json_parser *parser,
                           json_parser_callbacks *callbacks, const char *data,
                           size_t len) {
  const char *p = data;
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
        UPDATE_STATE(s_parse_object);
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
      } else if (ch == CB) {
        UPDATE_STATE(s_done);
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
      ARRAY_CALLBACK();
      UPDATE_STATE(s_parse_array);
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
      // TODO combine with s_parse_array_find_array_end_string_end
      if (ch == QM) {
        UPDATE_STATE(s_parse_array_find_object_end);
      } else if (ch == '\\') {
        p++; // Skip escaped characters.
      }
      break;
    }
    case s_parse_array_string: {
      // TODO combine with s_parse_array_find_array_end_string_end
      if (ch == QM) {
        UPDATE_STATE(s_parse_array_item_end);
        REEXECUTE();
      } else if (ch == '\\') {
        p++; // Skip escaped characters.
      }
      break;
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
        UPDATE_STATE(s_parse_array);
      }
      break;
    }
    case s_parse_object: {
      if (IS_WHITESPACE(ch) || ch == COMMA) {
        break;
      }
      if (ch == QM) {
        MARK_OBJECT_KEY_START(p + 1);
        UPDATE_STATE(s_parse_object_parse_key);
      } else if (ch == CCB) {
        UPDATE_STATE(s_done);
      } else {
        SET_ERRNO(ERRNO_INVALID_CHARACTER);
        goto error;
      }
      break;
    }
    case s_parse_object_parse_key: {
      // TODO combine with s_parse_array_find_array_end_string_end
      if (ch == QM) {
        UPDATE_STATE(s_parse_object_parse_key_end);
        REEXECUTE();
      } else if (ch == '\\') {
        p++; // Skip escaped characters.
      }
      break;
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
        UPDATE_STATE(s_parse_object_value_find_object_end);
        REEXECUTE();
      } else if (ch == OB) {
        UPDATE_STATE(s_parse_object_value_find_array_end);
        REEXECUTE();
      } else if (ch == 't') {
        UPDATE_STATE(s_parse_object_value_tr);
      } else if (ch == 'f') {
        UPDATE_STATE(s_parse_object_value_fa);
      } else if (ch == 'n') {
        UPDATE_STATE(s_parse_object_value_nu);
      } else if (IS_NUMERIC(ch)) {
        UPDATE_STATE(s_parse_object_value_numeric);
      }
      break;
    }
    case s_parse_object_value_string: {
      // TODO combine with s_parse_array_find_array_end_string_end
      if (ch == QM) {
        UPDATE_STATE(s_parse_object_value_end);
        REEXECUTE();
      } else if (ch == '\\') {
        p++; // Skip escaped characters.
      }
      break;
    }
    case s_parse_object_value_numeric: {
      if (ch == COMMA || ch == CCB || IS_WHITESPACE(ch)) {
        OBJECT_CALLBACK_NOADVANCE()
        UPDATE_STATE(s_parse_object);
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
      // TODO combine with s_parse_array_find_array_end_string_end
      if (ch == QM) {
        UPDATE_STATE(s_parse_object_value_find_array_end);
      } else if (ch == '\\') {
        p++; // Skip escaped characters.
      }
      break;
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
      // TODO combine with s_parse_array_find_array_end_string_end
      if (ch == QM) {
        UPDATE_STATE(s_parse_object_value_find_object_end);
      } else if (ch == '\\') {
        p++; // Skip escaped characters.
      }
      break;
    }
    case s_parse_object_value_end: {
      OBJECT_CALLBACK()
      UPDATE_STATE(s_parse_object);
      break;
    }
    case s_done: {
      if (p != (data + len - 1)) {
        SET_ERRNO(ERRNO_INVALID_CHARACTER);
        goto error;
      }
      break;
    }
    }
  }

  RETURN(len);

error:
  if (JSON_ERRNO(parser) == ERRNO_OK) {
    SET_ERRNO(ERRNO_UNKNOWN);
  }

  RETURN(p - data);
}

static int json_parser_typed_execute_json_object_cb(json_parser *parser,
                                                    const char *key,
                                                    size_t key_length,
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
        _data->parser, key, key_length, type, value, value_length);
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

size_t json_parser_typed_execute(json_parser *parser,
                                 json_parser_callbacks_typed *callbacks,
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
