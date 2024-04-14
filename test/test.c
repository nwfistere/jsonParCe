#include "parser.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifndef _WIN32
#include <sys/time.h>
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

static int on_array(json_parser *parser, unsigned int index, const char *value,
                    size_t value_length) {
  // printf("index: %d - value: %.*s\n", index, value_length, value);
  return 0;
}

static json_parser_callbacks cbs = {.on_object_key_value_pair = NULL,
                                    .on_array_value = on_array};

/* 8 gb */
static const int64_t kBytes = 8LL << 30;

static const char *data = " \
[ \
  [], \
  null, \
  true, \
  false, \
  \"string\", \
  { \
    \"array\": [], \
    \"null\": null, \
    \"string\": \"[]\", \
    \"unicode\": \"\u2022\", \
    \"escapedString\": \"This is a backslash: \\\\\", \
    \"specialCharacters\": \"This string contains special characters: \b\f\n\r\t\v\" \
  }, \
  \"This string has a \\u0011 control character\", \
  \"\\uD800\\uDF00\" \
] \
";

static const size_t data_len = sizeof(data) - 1;

static const int silent = 0;

int mutli();
int bench(int iter_count);

int main() {
  // json_parser parser;

  // json_parser_init(&parser);

  // json_parser_execute(&parser, &cbs, data, strlen(data));

  // return 0;
  return multi();
}

int bench(int iter_count) {
  json_parser parser;
  int i;
#ifndef _WIN32
  int err;
  struct timeval start;
  struct timeval end;
#else
  ULONGLONG start;
  ULONGLONG end;
#endif

  if (!silent) {
#ifndef _WIN32
    err = gettimeofday(&start, NULL);
    assert(err == 0);
#else
    start = GetTickCount64();
#endif
  }

  fprintf(stderr, "req_len=%d\n", (int)data_len);

  for (i = 0; i < iter_count; i++) {
    size_t parsed;
    json_parser_init(&parser);

    parsed = json_parser_execute(&parser, &cbs, data, data_len);
  }

  if (!silent) {
    double elapsed;
    double bw;
    double total;

#ifndef _WIN32
    err = gettimeofday(&end, NULL);
    assert(err == 0);
    elapsed = (double)(end.tv_sec - start.tv_sec) +
              (end.tv_usec - start.tv_usec) * 1e-6f;
#else
    end = GetTickCount64();
    elapsed = (double)(end - start) / 1000;
#endif

    total = (double)iter_count * data_len;
    bw = (double)total / elapsed;

    fprintf(stdout, "Benchmark result:\n");

    fprintf(stdout, "%.2f mb | %.2f mb/s | %.2f req/sec | %.2f s\n",
            (double)total / (1024 * 1024), bw / (1024 * 1024),
            (double)iter_count / elapsed, elapsed);

    fflush(stdout);
  }
  return 0;
}

int multi() {
  int64_t iterations = kBytes / (int64_t)data_len;
  return bench(iterations);
}
