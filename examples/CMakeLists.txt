project(examples)

add_executable(shallow_parse ./shallow_parsing.c)
add_executable(deep_parse ./deep_parsing.c)

target_include_directories(shallow_parse PRIVATE ../include)
target_compile_options(shallow_parse PRIVATE ${JSON_PARCE_COMPILE_OPTIONS})
target_link_options(shallow_parse PRIVATE ${JSON_PARCE_LINK_OPTIONS})
target_link_libraries(shallow_parse PRIVATE json_parce_static)

target_include_directories(deep_parse PRIVATE ../include)
target_compile_options(deep_parse PRIVATE ${JSON_PARCE_COMPILE_OPTIONS})
target_link_options(deep_parse PRIVATE ${JSON_PARCE_LINK_OPTIONS})
target_link_libraries(deep_parse PRIVATE json_parce_static)
