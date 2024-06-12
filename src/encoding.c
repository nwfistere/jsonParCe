#include "encoding.h"
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <corecrt.h>
#endif

int check_utf_bom(uint8_t *p) {
  uint8_t one = *p;
  uint8_t two = *(p + 1);
  uint8_t three = *(p + 2);
  uint8_t four = *(p + 3);

  if ((one == 0xEF) && (two == 0xBB) && (three == 0xBF)) {
    return UTF8;
  } else if ((one == 0xFE) && (two == 0xFF)) {
    return UTF16BE;
  } else if ((one == 0xFF) && (two == 0xFE) &&
             !((three == 0x00) && (four == 0x00))) {
    return UTF16LE;
  } else if ((one == 0x00) && (two == 0x00) && (three == 0xFE) &&
             (four == 0xFF)) {
    return UTF32BE;
  } else if ((one == 0xFF) && (two == 0xFE) && (three == 0x00) &&
             (four == 0x00)) {
    return UTF32LE;
  }

  return UNKNOWN;
};

// https://jameshfisher.com/2017/01/24/bitwise-check-for-zero-byte/
#define contains_zero_bytes_32(v)                                              \
  (((v) - (uint32_t)0x01010101) & ~(v) & (uint32_t)(0x80808080))

int check_json_byte_encoding(uint8_t *bytes) {
  int type = 0;
  if ((type = check_utf_bom(bytes)) > 0) {
    return type;
  } else {
    uint8_t one = *bytes;
    uint8_t two = *(bytes + 1);
    uint8_t three = *(bytes + 2);
    uint8_t four = *(bytes + 3);
    if (!(one | two | three)) {
      return UTF32BE;
    } else if (!(two | three | four)) {
      return UTF32LE;
    } else if (!(one | three)) {
      return UTF16BE;
    } else if (!(two | four)) {
      return UTF16LE;
    } else if (!contains_zero_bytes_32(((uint32_t *)bytes)[0])) {
      return UTF8;
    }
  }

  return UNKNOWN;
}

JSON_PARCE_API int get_file_info(const char *filepath, FILE **fp, int *encoding,
                                 size_t *file_size) {
  size_t nread = 0;
  uint8_t bytes[READ_SIZE];
  uint8_t *p = &bytes[0];

  *fp = fopen(filepath, "r");
  if (!*fp) {
    return -1;
  }

  fseek(*fp, 0, SEEK_END);
  *file_size = ftell(*fp);
  fseek(*fp, 0, SEEK_SET);

  if (*file_size < READ_SIZE) {
    // If the file size is less than the
    // READ_SIZE, just assume it's utf8 and continue on.
    *encoding = UTF8;
    return 0;
  }

  while ((nread = fread(p, 1, &bytes[READ_SIZE] - p, *fp)) > 0) {
    p += nread;
  }

  if (ferror(*fp)) {
    fclose(*fp);
    return -1;
  }

  fseek(*fp, 0, SEEK_SET);

  *encoding = check_json_byte_encoding(bytes);

  return 0;
}

static void do_set_local_locale(char **original_locale) {
  char *locale;
  if ((locale = setlocale(LC_CTYPE, NULL)) != NULL) {
    *original_locale = strdup(locale);
#ifdef _WIN32
    const char *new_locale = ".utf8";
#else
    const char *new_locale = "";
#endif
    if (setlocale(LC_CTYPE, new_locale) == NULL) {
      printf("Failed to set locale - LC_CTYPE\n");
    }
  }
}

static void do_unset_local_locale(char **original_locale) {
  if (setlocale(LC_CTYPE, *original_locale) == NULL) {
    // If unsetting fails, reset the locale to C default.
    setlocale(LC_CTYPE, "C");
  }
  free(*original_locale);
}

#define set_local_locale()                                                     \
  char *_original_locale = NULL;                                               \
  do_set_local_locale(&_original_locale);

#define unset_local_locale() do_unset_local_locale(&_original_locale);

#define flip_endianess(TYPE, VALUE, RESULT)                                    \
  if (sizeof(TYPE) == 2) {                                                     \
    RESULT = (((VALUE >> 8) | (VALUE << 8)) & 0xFFFF);                         \
  } else if (sizeof(TYPE) == 4) {                                              \
    RESULT = ((VALUE >> 24) & 0xff) | ((VALUE << 8) & 0xff0000) |              \
             ((VALUE >> 8) & 0xff00) | ((VALUE << 24) & 0xff000000);           \
  }

JSON_PARCE_API int c32strtomb(const char32_t *str, size_t len, int encoding,
                              char **out, size_t *out_sz) {
  set_local_locale();
  mbstate_t state = {0};
  *out = (char *)malloc(MB_CUR_MAX * (len + 1));
  char *p = *out;

  for (size_t n = 0; n < len; ++n) {
    char32_t tn;
    if (encoding == UTF32BE) {
      flip_endianess(char32_t, str[n], tn);
    } else {
      tn = str[n];
    }

    size_t rc = c32rtomb(p, tn, &state);

    if (rc == (size_t)-1) {
      return -1;
    }
    p += rc;
  }

  *out_sz = p - *out;
  (*out)[*out_sz] = U'\0';

  unset_local_locale();
  return 0;
}

JSON_PARCE_API int c16strtomb(const char16_t *str, size_t len, int encoding,
                              char **out, size_t *out_sz) {
  set_local_locale();
  mbstate_t state = {0};
  *out = (char *)malloc(MB_CUR_MAX * (len + 1));
  char *p = *out;

  for (size_t n = 0; n < len; ++n) {
    char16_t tn;
    if (encoding == UTF16BE) {
      flip_endianess(char16_t, str[n], tn);
    } else {
      tn = str[n];
    }
    size_t rc = c16rtomb(p, tn, &state);
    if (rc == (size_t)-1) {
      return -1;
    }
    p += rc;
  }

  *out_sz = p - *out;
  (*out)[(*out_sz)] = u'\0';

  unset_local_locale();
  return 0;
}

// Function to convert a hexadecimal digit to a decimal value
int hex_digit_to_int(char c) {
    if ('0' <= c && c <= '9') return c - '0';
    if ('a' <= c && c <= 'f') return 10 + c - 'a';
    if ('A' <= c && c <= 'F') return 10 + c - 'A';
    return -1; // Invalid hexadecimal digit
}

// Function to convert a Unicode escape sequence to a Unicode code point
unsigned int unicode_escape_to_codepoint(const char *str) {
    unsigned int value = 0;
    for (int i = 0; i < 4; i++) {
        int digit = hex_digit_to_int(str[i]);
        if (digit == -1) {
            return -1;
        }
        value = value * 16 + digit;
    }
    return value;
}

// Function to encode a Unicode code point into a UTF-8 byte sequence
int codepoint_to_utf8(unsigned int codepoint, char *output) {
    if (codepoint <= 0x7F) {
        output[0] = codepoint;
        return 1;
    } else if (codepoint <= 0x7FF) {
        output[0] = 0xC0 | (codepoint >> 6);
        output[1] = 0x80 | (codepoint & 0x3F);
        return 2;
    } else if (codepoint <= 0xFFFF) {
        output[0] = 0xE0 | (codepoint >> 12);
        output[1] = 0x80 | ((codepoint >> 6) & 0x3F);
        output[2] = 0x80 | (codepoint & 0x3F);
        return 3;
    } else if (codepoint <= 0x10FFFF) {
        output[0] = 0xF0 | (codepoint >> 18);
        output[1] = 0x80 | ((codepoint >> 12) & 0x3F);
        output[2] = 0x80 | ((codepoint >> 6) & 0x3F);
        output[3] = 0x80 | (codepoint & 0x3F);
        return 4;
    } else {
        fprintf(stderr, "Invalid Unicode code point: %X\n", codepoint);
        exit(EXIT_FAILURE);
    }
}

// Function to process the string and replace Unicode escape sequences
int process_unicode_escape_string(const char *input, char** output) {
  size_t len = strlen(input);
  *output = malloc(len * 4 + 1); // Allocate memory for the output string (worst case: every char is replaced by a 4-byte UTF-8 char)
  if (!output) {
    return 1;
  }

  const char *p = input;
  char *q = *output;

  while (*p) {
    if (*p == '\\' && *(p + 1) == 'u' && p + 6 <= input + len) {
      unsigned int codepoint1 = unicode_escape_to_codepoint(p + 2);
      p += 6;

      if (codepoint1 == -1) {
        free(*output);
        return -1;
      }

      if (0xD800 <= codepoint1 && codepoint1 <= 0xDBFF) {
        if (*p == '\\' && *(p + 1) == 'u' && p + 6 <= input + len) {
            // This is a high surrogate and the next one should be a low surrogate
            unsigned int codepoint2 = unicode_escape_to_codepoint(p + 2);

            if (codepoint2 == -1) {
              free(*output);
              return -2;
            }

            if (0xDC00 <= codepoint2 && codepoint2 <= 0xDFFF) {
                // Combine surrogate pair into a single code point
                codepoint1 = ((codepoint1 - 0xD800) << 10 | (codepoint2 - 0xDC00)) + 0x10000;
                p += 6; // Move past the second part of the surrogate pair
            } else {
                free(*output);
                return -2;
            }
        } else {
          // codepoint is an invalid high surrogate without a low surrogate.
          free(*output);
          return -1;
        }
      } else if (0xDC00 <= codepoint1 && codepoint1 <= 0xDFFF) {
        // invalid low surrogate without high surrogate.
        free(*output);
        return -1;
      }

      q += codepoint_to_utf8(codepoint1, q);
      } else {
          *q++ = *p++;
      }
  }
  *q = '\0'; // Null-terminate the output string

  return 0;
}

size_t strlen16(register const char16_t *string) {
  if (!string)
    return 0;
  register size_t len = (size_t)-1;
  while (string[++len])
    ;
  return len;
}

size_t strlen32(register const char32_t *string) {
  if (!string)
    return 0;
  register size_t len = (size_t)-1;
  while (string[++len])
    ;
  return len;
}