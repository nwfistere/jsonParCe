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

int check_json_byte_encoding(uint8_t *bytes) {
  int type = 0;
  if ((type = check_utf_bom(bytes)) > 0) {
    return type;
  } else {
    uint8_t one = *bytes;
    uint8_t two = *(bytes + 1);
    uint8_t three = *(bytes + 2);
    uint8_t four = *(bytes + 3);
    if (!(one & 0xFF) && !(two & 0xFF) & !(three & 0xFF)) {
      return UTF32BE;
    } else if (!(two & 0xFF) && !(three & 0xFF) & !(four & 0xFF)) {
      return UTF32LE;
    } else if (!(one & 0xFF) && !(three & 0xFF)) {
      return UTF16BE;
    } else if (!(two & 0xFF) && !(four & 0xFF)) {
      return UTF16LE;
    } else if ((one & 0xFF) && (two & 0xFF) && (three & 0xFF) &&
               (four & 0xFF)) {
      return UTF8;
    }
  }

  return UNKNOWN;
}

LIBRARY_API int get_file_encoding(const char *file) {
  size_t nread = 0;
  uint8_t bytes[READ_SIZE];
  uint8_t *p = &bytes[0];

  FILE *fp = fopen(file, "r");
  if (!fp) {
    return -1;
  }

  while ((nread = fread(p, 1, &bytes[READ_SIZE] - p, fp)) > 0) {
    p += nread;
  }

  if (ferror(fp)) {
    fclose(fp);
    return -1;
  }
  fclose(fp);

  return check_json_byte_encoding(bytes);
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

LIBRARY_API int c32strtomb(const char32_t *str, size_t len, char **out,
                           size_t *out_sz) {
  set_local_locale();
  mbstate_t state = {0};
  *out = (char *)malloc(MB_CUR_MAX * (len + 1));
  char *p = *out;

  for (size_t n = 0; n < len; ++n) {
    size_t rc = c32rtomb(p, str[n], &state);
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

LIBRARY_API int c16strtomb(const char16_t *str, size_t len, char **out,
                           size_t *out_sz) {
  set_local_locale();
  mbstate_t state = {0};
  *out = (char *)malloc(MB_CUR_MAX * (len + 1));
  char *p = *out;

  for (size_t n = 0; n < len; ++n) {
    size_t rc = c16rtomb(p, str[n], &state);
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