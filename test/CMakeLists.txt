set(TEST_PARSER_COMPILE_OPTIONS ${PARSER_COMPILE_OPTIONS})
# Disable warnings on test code
list(FILTER TEST_PARSER_COMPILE_OPTIONS EXCLUDE REGEX [/-][Ww].*)
list(APPEND TEST_PARSER_COMPILE_OPTIONS /W0)

add_executable(${PROJECT_NAME}_bench bench.c)
target_include_directories(${PROJECT_NAME}_bench PRIVATE ../include)
target_link_libraries(${PROJECT_NAME}_bench PRIVATE json_parser_static)
target_compile_options(${PROJECT_NAME}_bench PRIVATE ${TEST_PARSER_COMPILE_OPTIONS}) 

add_executable(${PROJECT_NAME}_test test.c)
target_include_directories(${PROJECT_NAME}_test PRIVATE ../include)
target_link_libraries(${PROJECT_NAME}_test PRIVATE json_parser_static)
target_compile_options(${PROJECT_NAME}_test PRIVATE ${TEST_PARSER_COMPILE_OPTIONS})