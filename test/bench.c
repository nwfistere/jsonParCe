#include "json_parce.h"
#include <stdint.h>
#include <stdio.h>
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
                                   .on_array_value = on_array};

/* 8 gb */
static const int64_t kBytes = 8LL << 30;

static const char array_data[] = " \
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
static const size_t array_data_len = sizeof(array_data) - 1;

static const char object_data[] = "   \
{\
  \"s_parse_objectnullValue\": null,\
  \"emptyString\": \"\",\
  \"escapedString\": \"This is a backslash: \\\\\",\
  \"multilineString\": \"Line 1\nLine 2\nLine 3\",\
  \"unicodeString\": \"\u2022 Unicode Bullet •\",\
  \"numberAsString\": \"42\",\
  \"booleanAsString\": \"true\",\
  \"arrayWithNullAndEmpty\": [null, \"\", \"value\",[[[[[[\"[]]]]]]]]]\"]]]]],[],[],[],[]]],\
  \"objectWithSpecialKeys\": {\
    \"\": \"Empty Key\",\
    \"special-key\": \"Special Key with Hyphen\",\
    \"123\": \"Numeric Key\"\
  },\
  \"deeplyNestedObject\": {\
    \"level1\": {\
      \"level2\":\
      {\
        \"level3\": {\
          \"level4\": {\
            \"value\": \"Deeply Nested Value\"\
          }\
        }\
      }\
    }\
  },\
  \"longString\": \"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.\"\
  \"integer\": 42, \
  \"floatingPoint\": 3.14,\
  \"scientificNotation\": 1.5e10,\
  \"negativeInteger\": -123,\
  \"negativeFloatingPoint\": -0.001, \
  \"\": \"\" \
}\
";
static const size_t object_data_len = sizeof(object_data) - 1;

static const int silent = 0;

int bench_array_and_object();
int bench(int iter_count, const char *data, size_t len);

int main() { return bench_array_and_object(); }

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

    parsed = json_parce_execute(&parser, &cbs, data, len);
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

int bench_array_and_object() {
  int64_t iterations = kBytes / (int64_t)array_data_len;
  bench(iterations, array_data, array_data_len);
  iterations = kBytes / (int64_t)object_data_len;
  bench(iterations, object_data, object_data_len);
  return 0;
}
