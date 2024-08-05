#include "encoding.h"
#include "json_parce.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
#include <assert.h>
#include <sys/time.h>
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

static int on_array(json_parce *parser, size_t index, JSON_TYPE type,
                    const char *value, size_t value_length) {
  return 0;
}

static json_parce_callbacks cbs = {.on_object_key_value_pair = NULL,
                                   .on_array_value = NULL};

/* 8 gb */
static const int64_t kBytes = 8LL << 30;

static const int silent = 0;

int bench_file(const char *file);
int bench(int iter_count, const char *data, size_t len);

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Usage:\n %s <file_to_test>", argv[0]);
    return 1;
  }

  return bench_file(argv[1]);
}

int bench(int iter_count, const char *data, size_t len) {
  json_parce parser;
  int i;
#ifndef _WIN32
  int err;
  struct timeval start;
  struct timeval end;
#else
  ULONGLONG start = 0;
  ULONGLONG end = 0;
#endif

  if (!silent) {
#ifndef _WIN32
    err = gettimeofday(&start, NULL);
    assert(err == 0);
#else
    start = GetTickCount64();
#endif
  }

  fprintf(stderr, "req_len=%d\n", (int)len);

  for (i = 0; i < iter_count; i++) {
    size_t parsed;
    json_parce_init(&parser);

    parsed = json_deep_parce_execute(&parser, &cbs, data, len);
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

    total = (double)iter_count * len;
    bw = (double)total / elapsed;

    fprintf(stdout, "Benchmark result:\n");

    fprintf(stdout, "%.2f mb | %.2f mb/s | %.2f req/sec | %.2f s\n",
            (double)total / (1024 * 1024), bw / (1024 * 1024),
            (double)iter_count / elapsed, elapsed);

    fflush(stdout);
  }
  return 0;
}

int bench_file(const char *file) {
  // int get_file_info(const char *filepath, FILE **fp, int *encoding, size_t
  // *file_size);
  FILE *fp = NULL;
  int encoding = 0;
  size_t file_size = 0;

  int retval = get_file_info(file, &fp, &encoding, &file_size);

  char *content = (char *)malloc((file_size + 1) * sizeof(char));
  char *p = &content[0];
  size_t nread = 0;

  while ((nread = fread(p, 1, &content[file_size] - p, fp)) > 0) {
    p += nread;
  }

  size_t content_len = p - &content[0];
  content[content_len] = '\0';
  int64_t iterations = kBytes / (int64_t)file_size;

  bench(iterations, content, content_len);

  return 0;
}
