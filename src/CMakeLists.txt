find_package(Boost REQUIRED)
include(Tree-Sitter)

setup_tree_sitter(Tree-Sitter)
setup_tree_sitter_lua(TREE_SITTER_LUA_SOURCES)

# core
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/core CORE_SRC_LIST)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/tree_sitter TREE_SITTER_WRAPPER_SRC_LIST)

set(LIB_SRC_LIST
    ${CORE_SRC_LIST}
    ${TREE_SITTER_WRAPPER_SRC_LIST}
    ${TREE_SITTER_LUA_SOURCES})

add_library(${PROJECT_NAME} SHARED ${LIB_SRC_LIST})
target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(${PROJECT_NAME} PRIVATE Tree-Sitter)

install(TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}
        LIBRARY DESTINATION lib)

# install includes separately (and not as PUBLIC_HEADERs) to preserver directory structure
# trailing / is important
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp")
install(EXPORT ${PROJECT_NAME}
        FILE ${PROJECT_NAME}Config.cmake
        DESTINATION lib/cmake/${PROJECT_NAME})
