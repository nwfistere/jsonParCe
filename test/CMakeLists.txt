set(TEST_JSON_PARCE_COMPILE_OPTIONS ${JSON_PARCE_COMPILE_OPTIONS})
set(TEST_JSON_PARCE_LINK_OPTIONS ${JSON_PARCE_LINK_OPTIONS})


list(FILTER TEST_JSON_PARCE_COMPILE_OPTIONS EXCLUDE REGEX [/-][Ww].*)
if(CMAKE_C_COMPILER_ID MATCHES "MSVC")
  # Disable warnings on test code
  list(APPEND TEST_JSON_PARCE_COMPILE_OPTIONS /W0)
endif()

if(JSON_PARCE_DEBUG_TEST)
  add_compile_definitions(JSON_PARCE_ENABLE_TEST_DEBUG)
endif()

add_executable(${PROJECT_NAME}_bench bench.c)
target_include_directories(${PROJECT_NAME}_bench PRIVATE ../include)
target_link_libraries(${PROJECT_NAME}_bench PRIVATE json_parce_static)
target_link_options(${PROJECT_NAME}_bench PRIVATE ${TEST_JSON_PARCE_LINK_OPTIONS}) 
target_compile_options(${PROJECT_NAME}_bench PRIVATE ${TEST_JSON_PARCE_COMPILE_OPTIONS}) 

add_executable(${PROJECT_NAME}_test_static test.c)
target_include_directories(${PROJECT_NAME}_test_static PRIVATE ../include)
target_link_libraries(${PROJECT_NAME}_test_static PRIVATE json_parce_static)
target_link_options(${PROJECT_NAME}_test_static PRIVATE ${TEST_JSON_PARCE_LINK_OPTIONS}) 
target_compile_options(${PROJECT_NAME}_test_static PRIVATE ${TEST_JSON_PARCE_COMPILE_OPTIONS})

if(JSON_PARCE_ENABLE_STRICT_MODE)
  target_compile_definitions(${PROJECT_NAME}_test_static PUBLIC JSON_PARCE_STRICT_MODE)
endif()

add_executable(${PROJECT_NAME}_test_shared test.c)
target_include_directories(${PROJECT_NAME}_test_shared PRIVATE ../include)
target_link_libraries(${PROJECT_NAME}_test_shared PRIVATE json_parce_shared)
target_link_options(${PROJECT_NAME}_test_shared PRIVATE ${TEST_JSON_PARCE_LINK_OPTIONS}) 
target_compile_options(${PROJECT_NAME}_test_shared PRIVATE ${TEST_JSON_PARCE_COMPILE_OPTIONS})
target_compile_definitions(${PROJECT_NAME}_test_shared PRIVATE LIBRARY_IMPORTS)

if(JSON_PARCE_ENABLE_STRICT_MODE)
  target_compile_definitions(${PROJECT_NAME}_test_shared PUBLIC JSON_PARCE_STRICT_MODE)
endif()

if(CMAKE_CONFIGURATION_TYPES)
  set(TEST_WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/test/$<CONFIG>)
else()
  set(TEST_WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/test)
endif()

add_test(NAME ${PROJECT_NAME}_test_static COMMAND ${PROJECT_NAME}_test_static WORKING_DIRECTORY ${TEST_WORKING_DIRECTORY})
add_test(NAME ${PROJECT_NAME}_test_shared COMMAND ${PROJECT_NAME}_test_shared WORKING_DIRECTORY ${TEST_WORKING_DIRECTORY})
# Add shared library to path for test environment

# Copy the encoding files over to the build directory.
if(CMAKE_CONFIGURATION_TYPES)
  set_tests_properties(${PROJECT_NAME}_test_shared PROPERTIES ENVIRONMENT_MODIFICATION PATH=path_list_append:${CMAKE_BINARY_DIR}/$<CONFIG>)
  set(OUTPUT_ENCODING_DIRECTORY "${CMAKE_BINARY_DIR}/test/$<CONFIG>/encodings" CACHE STRING "The output directory for the encoding files")
  set(OUTPUT_JSON_TESTING_DIRECTORY "${CMAKE_BINARY_DIR}/test/$<CONFIG>/test_parsing" CACHE STRING "The output directory for the test json files")
  set(OUTPUT_JSON_TRANSFORM_TESTING_DIRECTORY "${CMAKE_BINARY_DIR}/test/$<CONFIG>/test_transform" CACHE STRING "The output directory for the test transform json files")
else()
  set_tests_properties(${PROJECT_NAME}_test_shared PROPERTIES ENVIRONMENT_MODIFICATION PATH=path_list_append:${CMAKE_BINARY_DIR})
  set(OUTPUT_ENCODING_DIRECTORY "${CMAKE_BINARY_DIR}/test/encodings"  CACHE STRING "The output directory for the encoding files")
  set(OUTPUT_JSON_TESTING_DIRECTORY "${CMAKE_BINARY_DIR}/test/test_parsing" CACHE STRING "The output directory for the test json files")
  set(OUTPUT_JSON_TRANSFORM_TESTING_DIRECTORY "${CMAKE_BINARY_DIR}/test/test_transform" CACHE STRING "The output directory for the test transform json files")
endif()
add_custom_command(TARGET ${PROJECT_NAME}_test_static POST_BUILD COMMAND "${CMAKE_COMMAND}" -E copy_directory_if_different "${CMAKE_CURRENT_SOURCE_DIR}/encodings" "${OUTPUT_ENCODING_DIRECTORY}")
add_custom_command(TARGET ${PROJECT_NAME}_test_static POST_BUILD COMMAND "${CMAKE_COMMAND}" -E copy_directory_if_different "${jsontestsuite_SOURCE_DIR}/test_parsing" "${OUTPUT_JSON_TESTING_DIRECTORY}")
add_custom_command(TARGET ${PROJECT_NAME}_test_static POST_BUILD COMMAND "${CMAKE_COMMAND}" -E copy_directory_if_different "${jsontestsuite_SOURCE_DIR}/test_transform" "${OUTPUT_JSON_TRANSFORM_TESTING_DIRECTORY}")
