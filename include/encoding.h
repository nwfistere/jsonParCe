#ifndef JSON_PARCE_ENCODING_H
#define JSON_PARCE_ENCODING_H

#include <stdint.h> // uint8_t
#include <stdio.h>
#include <uchar.h>

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

#define READ_SIZE 4

typedef enum UTF {
  UNKNOWN = 0,
  UTF8 = 1,
  UTF16LE,
  UTF16BE,
  UTF32LE,
  UTF32BE
} UTF;

#define IS_UTF8(X) ((X) == UTF8)
#define IS_UTF16(X) ((X) == UTF16LE) || ((X) == UTF16BE)
#define IS_UTF32(X) ((X) == UTF32LE) || ((X) == UTF32BE)

// Expects an array of at least 4 bytes present.
int check_utf_bom(uint8_t *p);

// Expects an array of at least 4 bytes present.
int check_json_byte_encoding(uint8_t *bytes);

JSON_PARCE_API int get_file_info(const char *filepath, FILE **fp, int *encoding,
                                 size_t *file_size);

int hex_digit_to_int(char c);

int codepoint_to_utf8(unsigned int codepoint, char *output);

size_t decode_string(char *str, size_t len);

/**
 * Converts \uXXXX encoded characters to multibyte unicode characters.
 * Returns 0 on success
 * -1 when a character is encoded incorrectly or the high surrogate is invalid
 * -2 when the low surrogate is invalid.
 * 1 on malloc failure.
 */
JSON_PARCE_API int process_unicode_escape_string(const char *input,
                                                 char **output);

// https://stackoverflow.com/a/22128415
JSON_PARCE_API size_t strlen16(register const char16_t *string);
JSON_PARCE_API size_t strlen32(register const char32_t *string);

JSON_PARCE_API int strcmp16(const char16_t *lhs, const char16_t *rhs);
JSON_PARCE_API int strcmp32(const char32_t *lhs, const char32_t *rhs);

JSON_PARCE_API int c32strtomb(const char32_t *str, size_t len, int encoding,
                              char **out, size_t *out_len);
JSON_PARCE_API int c16strtomb(const char16_t *str, size_t len, int encoding,
                              char **out, size_t *out_len);

JSON_PARCE_API int mbstrtoc32(const char *str, size_t len, char32_t **out,
                              size_t *out_len);
JSON_PARCE_API int mbstrtoc16(const char *str, size_t len, char16_t **out,
                              size_t *out_len);

#endif // JSON_PARCE_ENCODING_H