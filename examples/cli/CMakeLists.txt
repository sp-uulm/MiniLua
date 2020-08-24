
aux_source_directory("${CMAKE_CURRENT_SOURCE_DIR}" CLI_SRC_LIST)
add_executable(${PROJECT_NAME}-cli ${CLI_SRC_LIST})
target_link_libraries(${PROJECT_NAME}-cli PUBLIC ${PROJECT_NAME})

install(TARGETS ${PROJECT_NAME}-cli RUNTIME)
