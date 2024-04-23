cmake_minimum_required(VERSION 3.26)

set(PARSER_COMPILE_SHARED_LIBRARY ON CACHE BOOL "Enables compiling shared library")
set(PARSER_COMPILE_STATIC_LIBRARY ON CACHE BOOL "Enables compiling static library")
set(PARSER_ENABLE_TEST ON CACHE BOOL "Enables compiling tests")
set(PARSER_ENABLE_TEST_COVERAGE ON CACHE BOOL "Enables test coverage")

project(json_parser C)

set(SRCS ./src/c_json_parser.c)


if(CMAKE_C_COMPILER_ID MATCHES "MSVC")
  # MSVC Warning 4820 warns about padded bytes in struct.
  # MSVC Warning 5045 warns about spectre protection
  set(PARSER_COMPILE_OPTIONS /nologo /Wall /WX /wd4820 /wd5045)
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    list(APPEND PARSER_COMPILE_OPTIONS /Ox)
  else()
    list(APPEND PARSER_COMPILE_OPTIONS /Od /Zi /RTCcsu /MP)
  endif() # CMAKE_BUILD_TYPE STREQUAL "Release"
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID MATCHES "GNU")
  set(PARSER_COMPILE_OPTIONS -Wall -Werror)
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    list(APPEND PARSER_COMPILE_OPTIONS -O3)
  else()
    if (PARSER_ENABLE_TEST_COVERAGE)
      list(APPEND PARSER_COMPILE_OPTIONS --coverage)
      add_link_options(--coverage)
    endif()
    list(APPEND PARSER_COMPILE_OPTIONS -g) # Seems redundant, at least for gcc (cmake already adds it?)
  endif()
endif() # CMAKE_C_COMPILER_ID

add_library(${PROJECT_NAME}_obj OBJECT ${SRCS})

target_include_directories(${PROJECT_NAME}_obj PRIVATE include)
target_compile_options(${PROJECT_NAME}_obj PRIVATE ${PARSER_COMPILE_OPTIONS})
if (PARSER_COMPILE_SHARED_LIBRARY AND (CMAKE_C_COMPILER_ID MATCHES "GNU"))
  target_compile_options(${PROJECT_NAME}_obj PUBLIC -fPIC)
endif()

if(PARSER_COMPILE_SHARED_LIBRARY)
  add_library(${PROJECT_NAME}_shared SHARED $<TARGET_OBJECTS:${PROJECT_NAME}_obj>)
  target_compile_options(${PROJECT_NAME}_shared PRIVATE ${PARSER_COMPILE_OPTIONS})
  # Export functions
  target_compile_definitions(${PROJECT_NAME}_shared PRIVATE LIBRARY_EXPORTS)
  target_compile_definitions(${PROJECT_NAME}_obj PRIVATE LIBRARY_EXPORTS)
  install(TARGETS ${PROJECT_NAME}_shared)
endif() # PARSER_COMPILE_SHARED_LIBRARY

if(PARSER_COMPILE_STATIC_LIBRARY)
  add_library(${PROJECT_NAME}_static STATIC $<TARGET_OBJECTS:${PROJECT_NAME}_obj>)
  target_compile_options(${PROJECT_NAME}_static PRIVATE ${PARSER_COMPILE_OPTIONS})
  set_target_properties(${PROJECT_NAME}_static PROPERTIES PUBLIC_HEADER "include/c_json_parser.h")
  install(TARGETS ${PROJECT_NAME}_static PUBLIC_HEADER DESTINATION include)
endif() # PARSER_COMPILE_STATIC_LIBRARY

if(PARSER_ENABLE_TEST)
  enable_testing()
  add_subdirectory(test)
endif()