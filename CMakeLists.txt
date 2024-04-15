cmake_minimum_required(VERSION 3.26)

set(PARSER_COMPILE_SHARED_LIBRARY ON CACHE BOOL "Enables compiling shared library")
set(PARSER_COMPILE_STATIC_LIBRARY ON CACHE BOOL "Enables compiling static library")
set(PARSER_ENABLE_TEST ON CACHE BOOL "Enables compiling tests")

project(json_parser C)

set(SRCS ./src/parser.c)


if(CMAKE_C_COMPILER_ID MATCHES "MSVC")
  set(PARSER_COMPILE_OPTIONS /nologo /Wall /WX)
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    list(APPEND PARSER_COMPILE_OPTIONS /Ox)
  else()
    list(APPEND PARSER_COMPILE_OPTIONS /Od /Zi /RTCcsu /MP)
  endif() # CMAKE_BUILD_TYPE STREQUAL "Release"
endif() # CMAKE_C_COMPILER_ID MATCHES "MSVC"

add_library(${PROJECT_NAME}_obj OBJECT ${SRCS})

target_include_directories(${PROJECT_NAME}_obj PRIVATE include)
target_compile_options(${PROJECT_NAME}_obj PRIVATE ${PARSER_COMPILE_OPTIONS})

if(PARSER_COMPILE_SHARED_LIBRARY)
  add_library(${PROJECT_NAME}_shared SHARED $<TARGET_OBJECTS:${PROJECT_NAME}_obj>)
  target_compile_options(${PROJECT_NAME}_shared PRIVATE ${PARSER_COMPILE_OPTIONS})
endif() # PARSER_COMPILE_SHARED_LIBRARY

if(PARSER_COMPILE_STATIC_LIBRARY)
  add_library(${PROJECT_NAME}_static STATIC $<TARGET_OBJECTS:${PROJECT_NAME}_obj>)
  target_compile_options(${PROJECT_NAME}_static PRIVATE ${PARSER_COMPILE_OPTIONS})
endif() # PARSER_COMPILE_STATIC_LIBRARY

if(PARSER_ENABLE_TEST)
  add_subdirectory(test)
endif()