#ifndef C_JSON_PARSER_ENCODING_H
#define C_JSON_PARSER_ENCODING_H

#include <stdint.h> // uint8_t
#include <stdio.h>
#include <uchar.h>

#ifdef _WIN32
  #ifdef C_JSON_PARSER_LIBRARY_EXPORTS
    #define C_JSON_PARSER_API __declspec(dllexport)
  #elif defined(C_JSON_PARSER_STATIC_LIBRARY)
    #define C_JSON_PARSER_API
  #else
    #define C_JSON_PARSER_API __declspec(dllimport)
  #endif
#else
  #define C_JSON_PARSER_API
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

C_JSON_PARSER_API int c32strtomb(const char32_t *str, size_t str_sz, int encoding,
                           char **out, size_t *out_sz);
C_JSON_PARSER_API int c16strtomb(const char16_t *str, size_t str_sz, int encoding,
                           char **out, size_t *out_sz);
C_JSON_PARSER_API int get_file_info(const char *filepath, FILE **fp, int *encoding,
                              size_t *file_size);

// https://stackoverflow.com/a/22128415
size_t strlen16(register const char16_t *string);
size_t strlen32(register const char32_t *string);

#endif // C_JSON_PARSER_ENCODING_H