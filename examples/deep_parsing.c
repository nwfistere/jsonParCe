#include "json_parce.h"
#include <assert.h>
#include <stdio.h>

#define str(x) #x
#define WIDTH str(%10)
#define PLS WIDTH str(.*) "s"
#define PS WIDTH "s"
#define PD WIDTH "d"

static void pretty_print(json_parce *parser, const char* key, size_t key_length, size_t index, int type, const char* value, size_t value_length) {
  printf("%10d", parser->current_depth->depth);
  if (parser->current_depth->key) {
    printf(PLS, (int)parser->current_depth->key_len, parser->current_depth->key);
  } else if (parser->current_depth->depth > 0) {
    printf(PD, (int)parser->current_depth->array_index);
  } else {
    if (key) {
      printf(PS, "{ROOT}");
    } else {
      printf(PS, "[ROOT]");
    }
  }
  if (key) {
    printf(PLS PS PD PLS "\n", (int)key_length, key, "-", type, (int)value_length, value);
  } else {
    printf(PS PD PD PLS "\n", "-", (int)index, type, (int)value_length, value);
  }
}

static int on_typed_object_value_cb(json_parce *parser, const char *key,
                                    size_t key_length, JSON_TYPE type,
                                    const char *value, size_t value_length) {

  pretty_print(parser, key, key_length, (size_t)-1, type, value, value_length);
  return 0;
}

static int on_typed_array_value_cb(json_parce *parser, size_t index,
                                   JSON_TYPE type, const char *value,
                                   size_t value_length) {
  pretty_print(parser, NULL, (size_t)-1, index, type, value, value_length);
  return 0;
}

static int on_start(json_parce *parser, JSON_TYPE type) {
  if (!parser->current_depth) // ROOT
    printf(PS PS PS PS PD PS PS "\n", "=","=", (type == OBJECT ? "{ROOT}" : "[ROOT]"), "=", type, "=", "START");
  else if (parser->object_key_mark)
    printf(PS PS PLS PS PD PS PS "\n", "=","=", (int)parser->object_key_len, parser->object_key_mark, "=", type, "=", "START");
  else 
    printf(PS PS PS PD PD PS PS "\n", "=", "=", "=", (int)parser->array_index, type, "=", "START");

  return 0;
}

static int on_end(json_parce *parser, JSON_TYPE type) {
  if (!parser->current_depth->parent) // ROOT
    printf(PS PS PS PS PD PS PS "\n", "=","=", (type == OBJECT ? "{ROOT}" : "[ROOT]"), "=", type, "=", "END");
  else
    printf(PS PS PS PS PD PS PS "\n", "=","=","=", "=", type, "=", "END");
  return 0;
}

static const char array_data[] = "[123,45.67,true,false,null,\"string\",[1,2,3],{\"a\":1,\"b\":false,\"c\":{\"d\":\"nested\"}},[{\"e\":0},{\"f\":\"g\"}],-12.34,{}]";
static const size_t array_data_len = sizeof(array_data) - 1;

static const char object_data[] = "{\"a\":123,\"b\":45.67,\"c\":true,\"d\":false,\"e\":null,\"f\":\"string\",\"g\":[1,2,3],\"h\":{\"i\":1,\"j\":false,\"k\":{\"l\":\"nested\"}},\"m\":[{\"n\":0},{\"o\":\"p\"}],\"q\":-12.34,\"r\":{}}";
static const size_t object_data_len = sizeof(object_data) - 1;

int main(void) {

  json_parce parser;
  json_parce_callbacks cbs = {
    .on_array_value = on_typed_array_value_cb,
    .on_object_key_value_pair = on_typed_object_value_cb,
    .on_start = on_start,
    .on_end = on_end
  };

  printf("%10s%10s%10s%10s%10s%10s\n", "Depth", "Parent", "Key", "Index",
         "Type", "Value");
  json_parce_init(&parser);
  size_t retval =
      json_deep_parce_execute(&parser, &cbs, array_data, array_data_len);
  assert(retval == array_data_len);
  (void)retval; // Make gcc happy.

  printf("\n");

  printf("%10s%10s%10s%10s%10s%10s\n", "Depth", "Parent", "Key", "Index",
         "Type", "Value");
  json_parce_init(&parser);
  retval = json_deep_parce_execute(&parser, &cbs, object_data, object_data_len);
  assert(retval == object_data_len);
  (void)retval; // Make gcc happy.

  return 0;
}