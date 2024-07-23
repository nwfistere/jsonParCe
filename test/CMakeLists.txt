set(TEST_JSON_PARCE_COMPILE_OPTIONS ${JSON_PARCE_COMPILE_OPTIONS})
set(TEST_JSON_PARCE_LINK_OPTIONS ${JSON_PARCE_LINK_OPTIONS})

include("../cmake/destination.config.cmake")

list(FILTER TEST_JSON_PARCE_COMPILE_OPTIONS EXCLUDE REGEX [/-][Ww].*)
if(CMAKE_C_COMPILER_ID MATCHES "MSVC")
  # Disable warnings on test code
  list(APPEND TEST_JSON_PARCE_COMPILE_OPTIONS /W0)
endif()

if(JSON_PARCE_ENABLE_STRICT_MODE)
  add_compile_definitions(JSON_PARCE_STRICT_MODE)
endif()

if(JSON_PARCE_DEBUG_TEST)
  add_compile_definitions(JSON_PARCE_ENABLE_TEST_DEBUG)
endif()

include_directories(../include)
add_link_options(${TEST_JSON_PARCE_LINK_OPTIONS})
add_compile_options(${TEST_JSON_PARCE_COMPILE_OPTIONS})

add_executable(${PROJECT_NAME}_bench bench.c)
target_link_libraries(${PROJECT_NAME}_bench PRIVATE json_parce_static)

add_executable(${PROJECT_NAME}_test_static test.c)
target_link_libraries(${PROJECT_NAME}_test_static PRIVATE json_parce_static)

add_executable(${PROJECT_NAME}_test_shared test.c)
target_link_libraries(${PROJECT_NAME}_test_shared PRIVATE json_parce_shared)
target_compile_definitions(${PROJECT_NAME}_test_shared PRIVATE LIBRARY_IMPORTS)

add_executable(${PROJECT_NAME}_test_jsontestsuite test_jsontestsuite.c)
target_link_libraries(${PROJECT_NAME}_test_jsontestsuite PRIVATE json_parce_shared)
target_compile_definitions(${PROJECT_NAME}_test_jsontestsuite PRIVATE LIBRARY_IMPORTS)

get_directory(FILEPATH ${CMAKE_BINARY_DIR}/test RESULT TEST_WORKING_DIRECTORY)

add_test(NAME ${PROJECT_NAME}_test_static COMMAND ${PROJECT_NAME}_test_static ${CMAKE_CURRENT_SOURCE_DIR}/encodings WORKING_DIRECTORY ${TEST_WORKING_DIRECTORY})
add_test(NAME ${PROJECT_NAME}_test_shared COMMAND ${PROJECT_NAME}_test_shared ${CMAKE_CURRENT_SOURCE_DIR}/encodings WORKING_DIRECTORY ${TEST_WORKING_DIRECTORY})
add_test(NAME ${PROJECT_NAME}_test_jsontestsuite COMMAND ${PROJECT_NAME}_test_jsontestsuite ${jsontestsuite_SOURCE_DIR}/test_parsing WORKING_DIRECTORY ${TEST_WORKING_DIRECTORY})

get_directory(FILEPATH ${CMAKE_BINARY_DIR} RESULT SHARED_LIBRARY_LOCATION)

set_tests_properties(${PROJECT_NAME}_test_shared PROPERTIES ENVIRONMENT_MODIFICATION PATH=path_list_append:${SHARED_LIBRARY_LOCATION})
set_tests_properties(${PROJECT_NAME}_test_jsontestsuite PROPERTIES ENVIRONMENT_MODIFICATION PATH=path_list_append:${SHARED_LIBRARY_LOCATION})
