# Depending on what generator/config we're using, binaries are built to different directories.
# e.g. <BUILD_DIR>/test/json_parce_test_shared vs <BUILD_DIR>/test/Debug/json_parce_test_shared
# I've tried to find a simple solution to this, but it may just come down to hardcoded checks
# That's what this file is for.

include(CMakeParseArguments)

#
# get_binary
#
# @param FILENAME - The filename of the file being found (e.g. libexample.so)
#
# @param FILEPATH - The (expected) filepath within the build directory of the file being found. (e.g. ${CMAKE_BINARY_DIR}/test)
#
function(get_binary)
  set(ARGS FILENAME FILEPATH RESULT)
  cmake_parse_arguments(GET_BINARY "" "${ARGS}" "" ${ARGN})

  # TODO: verify the "Debug" directory isn't created by any other generators.
  if (WIN32 AND CMAKE_C_COMPILER_ID MATCHES "MSVC")
    cmake_path(APPEND GET_BINARY_FILEPATH $<$<BOOL:$<CONFIG>>:$<CONFIG>> "${GET_BINARY_FILENAME}" OUTPUT_VARIABLE LOCAL_GET_BINARY_RESULT)
  else()
    cmake_path(APPEND GET_BINARY_FILEPATH "${GET_BINARY_FILENAME}" OUTPUT_VARIABLE LOCAL_GET_BINARY_RESULT)
  endif() # WIN32 and CMAKE_C_COMPILER_ID MATCHES "MSVC"

  set(${GET_BINARY_RESULT} ${LOCAL_GET_BINARY_RESULT})

  return(PROPAGATE ${GET_BINARY_RESULT})
endfunction() # get_binary

macro(get_directory)
  get_binary(${ARGN} FILENAME "")
endmacro()