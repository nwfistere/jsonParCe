# jsonParCe

## Description
A callback based json parser inspired by [nodejs/http-parser](https://github.com/nodejs/http-parser), built in C.

jsonParCe sets itself apart from most JSON libraries by offering a flexible foundation for building advanced logic. It prioritizes extracting all values from the entire JSON input, rather than focusing on specific keys or indices. A prime use case for this library would involve deserializing an entire JSON object into a struct, where each or most JSON values map to a corresponding data member. The library employs callbacks to access values from the original JSON object, without performing any datatype manipulation. As a result, all values are represented as const char* references with length from the input JSON.

## Features
 - Supports UTF-8, UTF-16LE, UTF-16BE, UTF-32LE, and UTF-32BE input*
    - *For non-utf8 encodings, the output is translated to utf-8 and freed at the end of the parsing method.
 - Supports both shallow and deep (depth-first) parsing.

## Usage
### Shallow parsing
```C
/* See full src in ./examples/shallow_parsing.c */
int main() {
  json_parser parser;
  json_parser_callbacks cbs = {
    .on_array_value = on_typed_array_value_cb,
    .on_object_key_value_pair = on_typed_object_value_cb
  };

  json_parser_init(&parser);
  size_t retval = json_parser_execute(&parser, &cbs, array_data, array_data_len);
  assert(retval == array_data_len);

  json_parser_init(&parser);
  retval = json_parser_execute(&parser, &cbs, object_data, object_data_len);
  assert(retval == object_data_len);

  return 0;
}
```
### Output
```bash
       Key     Index      Type               Value
         -         0         8              string
         -         1        32                null
         -         2        16                true
         -         3        16               false
         -         4         1  { "object": true }
         -         5         2           ["array"]
         -         6         4                1234

       Key     Index      Type               Value
     hello         -         8              world!
     bool1         -        16                true
     bool2         -        16               false
     array         -         2         [1,2,3,4,5]
    object         -         1     { "null": null}
      null         -        32                null
```
### Deep parsing
```C
/* See full src in ./examples/deep_parsing.c */
int main() {

  json_parser parser;
  json_parser_callbacks cbs = {
    .on_array_value = on_typed_array_value_cb,
    .on_object_key_value_pair = on_typed_object_value_cb
  };

  json_parser_init(&parser);
  size_t retval = json_deep_parser_execute(&parser, &cbs, array_data, array_data_len);
  assert(retval == array_data_len);

  json_parser_init(&parser);
  retval = json_deep_parser_execute(&parser, &cbs, object_data, object_data_len);
  assert(retval == object_data_len);

  return 0;
}
```

### Output
```bash
     Depth    Parent       Key     Index      Type     Value
         0    [ROOT]         -         0         8    string
         0    [ROOT]         -         1        32      null
         0    [ROOT]         -         2        16      true
         0    [ROOT]         -         3        16     false
         1         4    object         -        16      true
         1         5         -         0         8     array
         0    [ROOT]         -         6         4      1234

     Depth    Parent       Key     Index      Type     Value
         0    {ROOT}     hello         -         8    world!
         0    {ROOT}     bool1         -        16      true
         0    {ROOT}     bool2         -        16     false
         1     array         -         0         4         1
         1     array         -         1         4         2
         1     array         -         2         4         3
         1     array         -         3         4         4
         1     array         -         4         4         5
         1    object      null         -        32      null
         0    {ROOT}      null         -        32      null
```

## Building
jsonParCe uses cmake to compile
### Running cmake
```bash
# From the base of the source code
mkdir build
cmake -S . -B ./build # Add optional defines here.
cmake --build ./build
cmake --install --prefix <PREFIX>
```
### Optional defines
The following defines can be included in the initial cmake command. The option shown (ON/OFF) is the default.
 - `-DJSON_PARCE_COMPILE_SHARED_LIBRARY=ON`: Enables compiling of the static library
 - `-DJSON_PARCE_COMPILE_STATIC_LIBRARY=ON`: Enables compiling of the shared library
 - `-DJSON_PARCE_ENABLE_TEST=ON`: Enables compiling of the tests
 - `-DJSON_PARCE_ENABLE_TEST_COVERAGE=OFF`: Enables addition of the `--coverage` flag for gcov.
 - `-DJSON_PARCE_ENABLE_EXAMPLE_COMPILE=OFF`: Enables compiling of the examples directory.