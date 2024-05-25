project(json_path LANGUAGES CXX)

set(CPP_STANDARD 20)

add_executable(${PROJECT_NAME} main.cpp json_path.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE ../include)
target_link_libraries(${PROJECT_NAME} PRIVATE json_parce_shared)

cmake_print_variables(CMAKE_CXX_SIMULATE_ID)
cmake_print_variables(CMAKE_CXX_COMPILER_ID)
cmake_print_variables(CMAKE_CXX_COMPILER_FRONTEND_VARIANT)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)