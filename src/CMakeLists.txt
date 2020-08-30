find_package(Boost REQUIRED)
include(Tree-Sitter)

setup_tree_sitter(Tree-Sitter)
setup_tree_sitter_lua(TREE_SITTER_LUA_SOURCES)

# core
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/core CORE_SRC_LIST)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/tree-sitter TREE_SITTER_WRAPPER_SRC_LIST)

set(LIB_SRC_LIST
    ${CORE_SRC_LIST}
    ${TREE_SITTER_WRAPPER_SRC_LIST}
    ${TREE_SITTER_LUA_SOURCES})

add_library(${PROJECT_NAME} SHARED ${LIB_SRC_LIST})
target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:../include>
)
target_link_libraries(${PROJECT_NAME} PUBLIC Tree-Sitter)

# needed for installation
file(GLOB_RECURSE HEADER_LIST "${PROJECT_SOURCE_DIR}/include/*.hpp")
set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "${HEADER_LIST}")

install(TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}
        LIBRARY DESTINATION lib
        PUBLIC_HEADER DESTINATION include)
install(EXPORT ${PROJECT_NAME}
        FILE ${PROJECT_NAME}Config.cmake
        DESTINATION lib/cmake/${PROJECT_NAME})
