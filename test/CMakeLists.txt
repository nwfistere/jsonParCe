set(TEST_PARSER_COMPILE_OPTIONS ${PARSER_COMPILE_OPTIONS})
set(TEST_PARSER_LINK_OPTIONS ${PARSER_LINK_OPTIONS})


list(FILTER TEST_PARSER_COMPILE_OPTIONS EXCLUDE REGEX [/-][Ww].*)
if(CMAKE_C_COMPILER_ID MATCHES "MSVC")
  # Disable warnings on test code
  list(APPEND TEST_PARSER_COMPILE_OPTIONS /W0)
endif()

add_executable(${PROJECT_NAME}_bench bench.c)
target_include_directories(${PROJECT_NAME}_bench PRIVATE ../include)
target_link_libraries(${PROJECT_NAME}_bench PRIVATE json_parser_static)
target_link_options(${PROJECT_NAME}_bench PRIVATE ${TEST_PARSER_LINK_OPTIONS}) 
target_compile_options(${PROJECT_NAME}_bench PRIVATE ${TEST_PARSER_COMPILE_OPTIONS}) 

add_executable(${PROJECT_NAME}_test_static test.c)
target_include_directories(${PROJECT_NAME}_test_static PRIVATE ../include)
target_link_libraries(${PROJECT_NAME}_test_static PRIVATE json_parser_static)
target_link_options(${PROJECT_NAME}_test_static PRIVATE ${TEST_PARSER_LINK_OPTIONS}) 
target_compile_options(${PROJECT_NAME}_test_static PRIVATE ${TEST_PARSER_COMPILE_OPTIONS})

add_executable(${PROJECT_NAME}_test_shared test.c)
target_include_directories(${PROJECT_NAME}_test_shared PRIVATE ../include)
target_link_libraries(${PROJECT_NAME}_test_shared PRIVATE json_parser_shared)
target_link_options(${PROJECT_NAME}_test_shared PRIVATE ${TEST_PARSER_LINK_OPTIONS}) 
target_compile_options(${PROJECT_NAME}_test_shared PRIVATE ${TEST_PARSER_COMPILE_OPTIONS})
target_compile_definitions(${PROJECT_NAME}_test_shared PRIVATE LIBRARY_IMPORTS)

add_test(${PROJECT_NAME}_test_static ${PROJECT_NAME}_test_static)
add_test(${PROJECT_NAME}_test_shared ${PROJECT_NAME}_test_shared)
# Add shared library to path for test environment

# Copy the encoding files over to the build directory.
if(CMAKE_CONFIGURATION_TYPES)
  set_tests_properties(${PROJECT_NAME}_test_shared PROPERTIES ENVIRONMENT_MODIFICATION PATH=path_list_append:${CMAKE_BINARY_DIR}/test/$<CONFIG>)
  set(OUTPUT_ENCODING_DIRECTORY "${CMAKE_BINARY_DIR}/test/$<CONFIG>/encodings" CACHE STRING "The output directory for the encoding files")
  set(OUTPUT_JSON_TESTING_DIRECTORY "${CMAKE_BINARY_DIR}/test/$<CONFIG>/test_parsing" CACHE STRING "The output directory for the test json files")
else()
  set_tests_properties(${PROJECT_NAME}_test_shared PROPERTIES ENVIRONMENT_MODIFICATION PATH=path_list_append:${CMAKE_BINARY_DIR}/test)
  set(OUTPUT_ENCODING_DIRECTORY "${CMAKE_BINARY_DIR}/test/encodings"  CACHE STRING "The output directory for the encoding files")
  set(OUTPUT_JSON_TESTING_DIRECTORY "${CMAKE_BINARY_DIR}/test/test_parsing" CACHE STRING "The output directory for the test json files")
endif()
add_custom_command(TARGET ${PROJECT_NAME}_test_static POST_BUILD COMMAND "${CMAKE_COMMAND}" -E copy_directory_if_different "${CMAKE_CURRENT_SOURCE_DIR}/encodings" "${OUTPUT_ENCODING_DIRECTORY}")
add_custom_command(TARGET ${PROJECT_NAME}_test_static POST_BUILD COMMAND "${CMAKE_COMMAND}" -E copy_directory_if_different "${jsontestsuite_SOURCE_DIR}/test_parsing" "${OUTPUT_JSON_TESTING_DIRECTORY}")
