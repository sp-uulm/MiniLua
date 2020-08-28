find_package(Boost REQUIRED)

FetchContent_Declare(
    Tree-Sitter
    GIT_REPOSITORY https://github.com/tree-sitter/tree-sitter.git
    GIT_TAG        0.16.9)
FetchContent_MakeAvailable(Tree-Sitter)

add_custom_command(OUTPUT libtree-sitter.a
    COMMAND make -C "${tree-sitter_SOURCE_DIR}" libtree-sitter.a
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Building Tree-Sitter"
    )
add_custom_target(Tree-Sitter-Static-Lib
    DEPENDS libtree-sitter.a)
add_library(Tree-Sitter STATIC IMPORTED)
set_target_properties(Tree-Sitter
    PROPERTIES IMPORTED_LOCATION "${tree-sitter_SOURCE_DIR}/libtree-sitter.a")
target_include_directories(Tree-Sitter INTERFACE "${tree-sitter_SOURCE_DIR}/lib/include")
add_dependencies(Tree-Sitter Tree-Sitter-Static-Lib)

FetchContent_Declare(
    Tree-Sitter-Lua
    GIT_REPOSITORY https://github.com/Azganoth/tree-sitter-lua.git
    GIT_TAG        v1.6.1)
FetchContent_MakeAvailable(Tree-Sitter-Lua)
# add_library(Tree-Sitter-Lua STATIC "${tree-sitter-lua_SOURCE_DIR}/src/parser.c")
# target_link_libraries(Tree-Sitter-Lua PUBLIC Tree-Sitter)
# target_include_directories(Tree-Sitter-Lua PUBLIC
#     $<BUILD_INTERFACE:${tree-sitter-lua_SOURCE_DIR}/src>)
# add_dependencies(Tree-Sitter-Lua Tree-Sitter)

# core
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/core CORE_SRC_LIST)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/tree-sitter TREE_SITTER_WRAPPER_SRC_LIST)

set(LIB_SRC_LIST
    ${CORE_SRC_LIST}
    ${TREE_SITTER_WRAPPER_SRC_LIST}
    "${tree-sitter-lua_SOURCE_DIR}/src/parser.c"
    "${tree-sitter-lua_SOURCE_DIR}/src/scanner.cc")

add_library(${PROJECT_NAME} SHARED ${LIB_SRC_LIST})
target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:../include>
)
target_link_libraries(${PROJECT_NAME} PUBLIC Tree-Sitter)

# needed for installation
file(GLOB HEADER_LIST CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/include/*.h")
set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "${HEADER_LIST}")

install(TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}
        LIBRARY DESTINATION lib)
install(EXPORT ${PROJECT_NAME}
        FILE ${PROJECT_NAME}Config.cmake
        DESTINATION lib/cmake/${PROJECT_NAME})
