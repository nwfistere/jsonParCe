add_executable(${PROJECT_NAME}_test test.c)
target_include_directories(${PROJECT_NAME}_test PUBLIC ../include)
target_link_libraries(${PROJECT_NAME}_test PUBLIC json_parser_static)