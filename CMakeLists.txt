cmake_minimum_required(VERSION 3.26)

set(PARSER_COMPILE_SHARED_LIBRARY ON CACHE BOOL "Enables compiling shared library")
set(PARSER_COMPILE_STATIC_LIBRARY ON CACHE BOOL "Enables compiling static library")
set(PARSER_ENABLE_TEST ON CACHE BOOL "Enables compiling tests")

project(json_parser C)

set(SRCS ./src/parser.c)

add_library(${PROJECT_NAME}_obj OBJECT ${SRCS})

target_include_directories(${PROJECT_NAME}_obj PUBLIC include)

if(PARSER_COMPILE_SHARED_LIBRARY)
  add_library(${PROJECT_NAME}_shared SHARED $<TARGET_OBJECTS:${PROJECT_NAME}_obj>)
endif() # PARSER_COMPILE_SHARED_LIBRARY

if(PARSER_COMPILE_STATIC_LIBRARY)
  add_library(${PROJECT_NAME}_static STATIC $<TARGET_OBJECTS:${PROJECT_NAME}_obj>)
endif() # PARSER_COMPILE_STATIC_LIBRARY

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    compile_options(${PROJECT_NAME}_obj PUBLIC /W4 /Ox)
  endif()
else()
  if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    compile_options(${PROJECT_NAME}_obj PUBLIC /W4 /Od /Zi /ZI /RTC1 /RTCs /RTCsuc /MP)
  endif()
endif()

if(PARSER_ENABLE_TEST)
  add_subdirectory(test)
endif()