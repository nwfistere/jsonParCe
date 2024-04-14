cmake_minimum_required(VERSION 3.26)

set(PARSER_COMPILE_SHARED_LIBRARY ON CACHE BOOL "Enables compiling shared library")
set(PARSER_COMPILE_STATIC_LIBRARY ON CACHE BOOL "Enables compiling static library")

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
