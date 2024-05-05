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
Index: [0] Value type: 4 Value: <string>
Index: [1] Value type: 6 Value: <null>
Index: [2] Value type: 5 Value: <true>
Index: [3] Value type: 5 Value: <false>
Index: [4] Value type: 1 Value: <{ "object": true }>
Index: [5] Value type: 2 Value: <["array"]>
Index: [6] Value type: 3 Value: <1234>
Key: "hello" Value type: 4 Value: <world!>
Key: "bool1" Value type: 5 Value: <true>
Key: "bool2" Value type: 5 Value: <false>
Key: "array" Value type: 2 Value: <[1,2,3,4,5]>
Key: "object" Value type: 1 Value: <{ "null": null}>
Key: "null" Value type: 6 Value: <null>
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
     Depth    Parent       Key     Index     Value      Type
         0    [ROOT]         -         0    string         4
         0    [ROOT]         -         1      null         6
         0    [ROOT]         -         2      true         5
         0    [ROOT]         -         3     false         5
         1         4    object         -      true         5
         1         5         -         0     array         4
         0    [ROOT]         -         6      1234         3

     Depth    Parent       Key     Index     Value      Type
         0    {ROOT}     hello         -    world!         4
         0    {ROOT}     bool1         -      true         5
         0    {ROOT}     bool2         -     false         5
         1     array         -         0         1         3
         1     array         -         1         2         3
         1     array         -         2         3         3
         1     array         -         3         4         3
         1     array         -         4         5         3
         1    object      null         -      null         6
         0    {ROOT}      null         -      null         6
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
 - `-DPARSER_COMPILE_SHARED_LIBRARY=ON`: Enables compiling of the static library
 - `-DPARSER_COMPILE_STATIC_LIBRARY=ON`: Enables compiling of the shared library
 - `-DPARSER_ENABLE_TEST=ON`: Enables compiling of the tests
 - `-DPARSER_ENABLE_TEST_COVERAGE=OFF`: Enables addition of the `--coverage` flag for gcov.
 - `-DPARSER_ENABLE_EXAMPLE_COMPILE=OFF`: Enables compiling of the examples directory.