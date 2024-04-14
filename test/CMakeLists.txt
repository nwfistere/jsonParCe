add_executable(${PROJECT_NAME}_bench bench.c)
target_include_directories(${PROJECT_NAME}_bench PUBLIC ../include)
target_link_libraries(${PROJECT_NAME}_bench PUBLIC json_parser_static)

add_executable(${PROJECT_NAME}_test test.c)
target_include_directories(${PROJECT_NAME}_test PUBLIC ../include)
target_link_libraries(${PROJECT_NAME}_test PUBLIC json_parser_static)