# creates a target that contains Tree-Sitter
# should probably be called like this:
# setup_tree_sitter(Tree-Sitter)
function(setup_tree_sitter TARGET_NAME)
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
    add_custom_target(${TARGET_NAME}-Static-Lib
        DEPENDS libtree-sitter.a)
    add_library(${TARGET_NAME} STATIC IMPORTED)
    set_target_properties(${TARGET_NAME}
        PROPERTIES IMPORTED_LOCATION "${tree-sitter_SOURCE_DIR}/libtree-sitter.a")
    target_include_directories(Tree-Sitter INTERFACE "${tree-sitter_SOURCE_DIR}/lib/include")
    add_dependencies(${TARGET_NAME} ${TARGET_NAME}-Static-Lib)
endfunction()

# returns the sources needed to use Tree-Sitter-Lua
function(setup_tree_sitter_lua SOURCES_LIST)
    FetchContent_Declare(
        Tree-Sitter-Lua
        GIT_REPOSITORY https://github.com/Azganoth/tree-sitter-lua.git
        GIT_TAG        v1.6.1)
    FetchContent_MakeAvailable(Tree-Sitter-Lua)

    set(${SOURCES_LIST}
        "${tree-sitter-lua_SOURCE_DIR}/src/parser.c"
        "${tree-sitter-lua_SOURCE_DIR}/src/scanner.cc"
        PARENT_SCOPE)
endfunction()
