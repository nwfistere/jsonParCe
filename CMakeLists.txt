cmake_minimum_required(VERSION 3.26)

set(JSON_PARCE_COMPILE_SHARED_LIBRARY ON CACHE BOOL "Enables compiling shared library")
set(JSON_PARCE_COMPILE_STATIC_LIBRARY ON CACHE BOOL "Enables compiling static library")
set(JSON_PARCE_ENABLE_TEST OFF CACHE BOOL "Enables compiling tests")
set(JSON_PARCE_ENABLE_TEST_COVERAGE OFF CACHE BOOL "Enables test coverage")
set(JSON_PARCE_ENABLE_EXAMPLE_COMPILE OFF CACHE BOOL "Enables compiling examples")
set(JSON_PARCE_ENABLE_SANITIZERS OFF CACHE BOOL "Enables compiling with sanitizer flags")
set(JSON_PARCE_ENABLE_STRICT_MODE OFF CACHE BOOL "Enables compiling with strict json syntax mode (will slow down parsing)")
set(JSON_PARCE_DEBUG_TEST OFF CACHE BOOL "Enables verbose logging during test.")


project(json_parce LANGUAGES C VERSION 0.0.5)
set(C_STANDARD 11)

include(FetchContent)

set(SRCS ./src/json_parce.c ./src/encoding.c)

FetchContent_Declare(
  JSONTestSuite
  GIT_REPOSITORY https://github.com/nst/JSONTestSuite.git
  GIT_TAG        master
)

if(CMAKE_C_COMPILER_ID MATCHES "MSVC")
  # NOTE /utf-8 flag. This library runs with the utf8 encoding by default.
  # MSVC Warning 4820 warns about padded bytes in struct.
  # MSVC Warning 5045 warns about spectre protection
  # MSVC Warning 4996 warns about using strdup
  # MSVC Warning 4127 warns about constant conditional statements.
  # MSVC Warning 4333 warns about shifting too far
  # MSVC Warning 4711 warns about auto-inline of non-inlined functions during optimization.
  # MSVC Warning 4710 warns about not inlining a specific function
  # C4476, 4474 testing
  set(JSON_PARCE_COMPILE_OPTIONS /nologo /Wall /WX /utf-8 /wd4820 /wd5045 /wd4996 /wd4127 /wd4333 /wd4711 /wd4710)
  list(APPEND JSON_PARCE_COMPILE_DEFINITIONS /D_CRT_SECURE_NO_WARNINGS)
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    # list(APPEND JSON_PARCE_COMPILE_OPTIONS /Ox)
    # string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
    list(APPEND JSON_PARCE_COMPILE_OPTIONS /Od /Zi /RTCcsu /MP)
  endif() # CMAKE_BUILD_TYPE STREQUAL "Release"
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID MATCHES "GNU")
  set(JSON_PARCE_COMPILE_OPTIONS -Wall -Werror)
  # Running clang on windows
  if (CMAKE_C_COMPILER_ID MATCHES "Clang" AND CMAKE_C_SIMULATE_ID MATCHES "MSVC")
    # Disable the posix name deprecated warnings.
    list(APPEND JSON_PARCE_COMPILE_DEFINITIONS -D_CRT_SECURE_NO_WARNINGS)
    list(APPEND JSON_PARCE_COMPILE_OPTIONS -Wno-deprecated-declarations)
  endif()
  
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    list(APPEND JSON_PARCE_COMPILE_OPTIONS -O3)
  else()
  endif()
endif() # CMAKE_C_COMPILER_ID

if (JSON_PARCE_ENABLE_TEST_COVERAGE AND (CMAKE_C_COMPILER_ID MATCHES "GNU" OR CMAKE_C_COMPILER_ID MATCHES "Clang"))
  list(APPEND JSON_PARCE_COMPILE_OPTIONS --coverage)
  list(APPEND JSON_PARCE_LINK_OPTIONS --coverage)
endif()

if(JSON_PARCE_ENABLE_SANITIZERS)
  if(CMAKE_C_COMPILER_ID MATCHES "GNU")
    set(C_OPTIONS
      -fno-omit-frame-pointer
      -fsanitize=address
      -fno-optimize-sibling-calls
      -fsanitize=undefined
      -fsanitize=float-cast-overflow
      -fsanitize-address-use-after-scope
      -fno-sanitize-recover
      -fsanitize=leak
    )
    list(APPEND JSON_PARCE_COMPILE_OPTIONS ${C_OPTIONS})
    list(APPEND JSON_PARCE_LINK_OPTIONS ${C_OPTIONS})
  elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
    if (NOT (CMAKE_C_SIMULATE_ID MATCHES "MSVC"))
      set (C_OPTIONS
        -fsanitize=address
        -fsanitize-address-use-after-scope
        -fno-omit-frame-pointer
        -fsanitize=undefined
        -fsanitize=float-cast-overflow
        -fno-sanitize-recover
        -O1
      )
    endif()
    list(APPEND JSON_PARCE_COMPILE_OPTIONS ${C_OPTIONS})
    list(APPEND JSON_PARCE_LINK_OPTIONS ${C_OPTIONS})
  endif() # CMAKE_C_COMPILER_ID MATCHES
endif() # JSON_PARCE_ENABLE_SANITIZERS

if(JSON_PARCE_COMPILE_SHARED_LIBRARY)
  list(APPEND JSON_PARCE_INSTALL_TARGETS ${PROJECT_NAME}_shared)
  set(JSON_PARCE_SHARED_COMPILE_DEFINITIONS ${JSON_PARCE_COMPILE_DEFINITIONS})
  list(APPEND JSON_PARCE_SHARED_COMPILE_DEFINITIONS JSON_PARCE_LIBRARY_EXPORTS)

  add_library(${PROJECT_NAME}_shared SHARED ${SRCS})
  target_include_directories(${PROJECT_NAME}_shared PRIVATE include)
  target_compile_options(${PROJECT_NAME}_shared PRIVATE ${JSON_PARCE_COMPILE_OPTIONS})
  target_link_options(${PROJECT_NAME}_shared PRIVATE ${JSON_PARCE_LINK_OPTIONS})
  target_compile_definitions(${PROJECT_NAME}_shared PUBLIC ${JSON_PARCE_SHARED_COMPILE_DEFINITIONS})
  if(JSON_PARCE_ENABLE_STRICT_MODE)
    target_compile_definitions(${PROJECT_NAME}_shared PUBLIC JSON_PARCE_STRICT_MODE)
  endif()
endif() # JSON_PARCE_COMPILE_SHARED_LIBRARY

if(JSON_PARCE_COMPILE_STATIC_LIBRARY)
  list(APPEND JSON_PARCE_INSTALL_TARGETS ${PROJECT_NAME}_static)
  set(JSON_PARCE_STATIC_COMPILE_DEFINITIONS ${JSON_PARCE_COMPILE_DEFINITIONS})
  list(APPEND JSON_PARCE_STATIC_COMPILE_DEFINITIONS JSON_PARCE_STATIC_LIBRARY)

  add_library(${PROJECT_NAME}_static STATIC ${SRCS})
  target_include_directories(${PROJECT_NAME}_static PRIVATE include)
  target_compile_options(${PROJECT_NAME}_static PRIVATE ${JSON_PARCE_COMPILE_OPTIONS})
  target_link_options(${PROJECT_NAME}_static PRIVATE ${JSON_PARCE_LINK_OPTIONS})
  target_compile_definitions(${PROJECT_NAME}_static PUBLIC ${JSON_PARCE_STATIC_COMPILE_DEFINITIONS})
  if(JSON_PARCE_ENABLE_STRICT_MODE)
    target_compile_definitions(${PROJECT_NAME}_static PUBLIC JSON_PARCE_STRICT_MODE)
  endif()
  set_target_properties(${PROJECT_NAME}_static PROPERTIES PUBLIC_HEADER "include/json_parce.h")
endif() # JSON_PARCE_COMPILE_STATIC_LIBRARY

if(JSON_PARCE_ENABLE_TEST)
  FetchContent_MakeAvailable(JSONTestSuite)
  enable_testing()
  add_subdirectory(test)
endif()

if(JSON_PARCE_ENABLE_EXAMPLE_COMPILE)
  add_subdirectory(examples)
endif()


# install targets
include(GNUInstallDirs) # For the CMAKE_INSTALL_* variables
include(CMakePackageConfigHelpers) # For the configure_package_config_file command

install(TARGETS
  ${JSON_PARCE_INSTALL_TARGETS}
  EXPORT JsonParCeTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
  FILES
  ./include/json_parce.h
  ./include/encoding.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
  EXPORT JsonParCeTargets
  FILE JsonParCeTargets.cmake
  NAMESPACE JsonParCe::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/JsonParCe
)

set(INCLUDE_INSTALL_DIR include)
set(LIBRARY_INSTALL_DIR lib)
set(BINARY_INSTALL_DIR bin)

# Create config file and install it.
configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Config.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/JsonParCeConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/JsonParCe
  PATH_VARS
  INCLUDE_INSTALL_DIR
  LIBRARY_INSTALL_DIR
  BINARY_INSTALL_DIR
)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/JsonParCeConfigVersion.cmake"
  VERSION "${VERSION}"
  COMPATIBILITY ExactVersion
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/JsonParCeConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/JsonParCeConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/JsonParCe
)

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE" DESTINATION .)

set(CPACK_PACKAGE_NAME JsonParCe)
set(CPACK_PACKAGE_VENDOR nwfistere)
set(CPACK_PACKAGE_CONTACT "Nick Fistere")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A callback based json parser.")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-v${CMAKE_PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}")

include(CPack)


