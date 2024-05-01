project(examples)

add_executable(shallow_parse ./shallow_parsing.c)
add_executable(deep_parse ./deep_parsing.c)

target_include_directories(shallow_parse PRIVATE ../include)
target_include_directories(deep_parse PRIVATE ../include)

target_link_libraries(shallow_parse PRIVATE json_parser_static)
target_link_libraries(deep_parse PRIVATE json_parser_static)
