set(TEST_PARSER_COMPILE_OPTIONS ${PARSER_COMPILE_OPTIONS})


list(FILTER TEST_PARSER_COMPILE_OPTIONS EXCLUDE REGEX [/-][Ww].*)
if(CMAKE_C_COMPILER_ID MATCHES "MSVC")
  # Disable warnings on test code
  list(APPEND TEST_PARSER_COMPILE_OPTIONS /W0)
endif()

add_executable(${PROJECT_NAME}_bench bench.c)
target_include_directories(${PROJECT_NAME}_bench PRIVATE ../include)
target_link_libraries(${PROJECT_NAME}_bench PRIVATE json_parser_static)
target_compile_options(${PROJECT_NAME}_bench PRIVATE ${TEST_PARSER_COMPILE_OPTIONS}) 

add_executable(${PROJECT_NAME}_test_static test.c)
target_include_directories(${PROJECT_NAME}_test_static PRIVATE ../include)
target_link_libraries(${PROJECT_NAME}_test_static PRIVATE json_parser_static)
target_compile_options(${PROJECT_NAME}_test_static PRIVATE ${TEST_PARSER_COMPILE_OPTIONS})

add_executable(${PROJECT_NAME}_test_shared test.c)
target_include_directories(${PROJECT_NAME}_test_shared PRIVATE ../include)
target_link_libraries(${PROJECT_NAME}_test_shared PRIVATE json_parser_shared)
target_compile_options(${PROJECT_NAME}_test_shared PRIVATE ${TEST_PARSER_COMPILE_OPTIONS})
target_compile_definitions(${PROJECT_NAME}_test_shared PRIVATE LIBRARY_IMPORTS)

add_test(${PROJECT_NAME}_test_static ${PROJECT_NAME}_test_static)
add_test(${PROJECT_NAME}_test_shared ${PROJECT_NAME}_test_shared)
# Add shared library to path for test environment
set_tests_properties(${PROJECT_NAME}_test_shared PROPERTIES ENVIRONMENT_MODIFICATION PATH=path_list_append:${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})