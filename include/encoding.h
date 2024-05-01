#ifndef C_JSON_ENCODING_H
#define C_JSON_ENCODING_H
#define _CRT_SECURE_NO_WARNINGS

#include <stdint.h> // uint8_t
#include <stdio.h>
#include <stdlib.h>
#include <uchar.h>

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

LIBRARY_API int c32strtomb(const char32_t *str, size_t str_sz, int encoding,
                           char **out, size_t *out_sz);
LIBRARY_API int c16strtomb(const char16_t *str, size_t str_sz, int encoding,
                           char **out, size_t *out_sz);
LIBRARY_API int get_file_encoding(const char *file);

// https://stackoverflow.com/a/22128415
size_t strlen16(register const char16_t *string);
size_t strlen32(register const char32_t *string);

#endif // C_JSON_ENCODING_H