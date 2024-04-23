#ifndef C_JSON_ENCODING_H
#define C_JSON_ENCODING_H

#include <errno.h>
#include <stdint.h> // uint8_t
#include <stdio.h>

#define READ_SIZE 8

typedef enum UTF {
  UNKNOWN = 0,
  UTF8 = 1,
  UTF16LE,
  UTF16BE,
  UTF32LE,
  UTF32BE
} UTF;

// Expects an array of at least 4 bytes present.
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

// Expects an array of at least 4 bytes present.
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

/*
int check_json_encoding(char* file) {
  FILE* fp = fopen(file, "rb");
  if (!fp) {
    fprintf(stderr, "Failed to open file <%s>, errno: %d", file, errno);
    return errno;
  }

  uint8_t bytes[READ_SIZE + 1] = { 0 };
  uint8_t *p = &bytes[0];
  size_t nread;

  while ((nread = fread(p, 1, 1, fp)) > 0) {
    p += nread;
    if (p == &bytes[READ_SIZE]) {
      break;
    }
  }

  for (int i = 0; i < READ_SIZE; i++) {
    printf("%d: %02hhx\n", i + 1, bytes[i]);
  }

  int type;
  if ((type = check_json_byte_encoding(bytes))) {
    printf("Type: %d\n", type);
  }

  if (ferror(fp)) {
    fprintf(stderr, "Error while reading...");
    fclose(fp);
    return 4;
  }
  fclose(fp);
  return 0;
}*/

#endif // C_JSON_ENCODING_H