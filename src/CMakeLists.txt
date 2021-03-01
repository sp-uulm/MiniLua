find_package(Boost REQUIRED)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} PUBLIC_INTERFACE_IMPL)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/details PRIVATE_IMPL)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/core CORE_SRC_LIST)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/tree_sitter TREE_SITTER_WRAPPER_SRC_LIST)

set(LIB_SRC_LIST
    ${PUBLIC_INTERFACE_IMPL}
    ${PRIVATE_IMPL}
    ${CORE_SRC_LIST}
    ${TREE_SITTER_WRAPPER_SRC_LIST})

# statically link the stdlib lua files
add_custom_command(OUTPUT stdlib.lua.o
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/lua/stdlib.lua
    COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR}/lua && ld -r -b binary -o ${CMAKE_CURRENT_BINARY_DIR}/stdlib.lua.o stdlib.lua
    COMMAND objcopy --rename-section .data=.rodata,alloc,load,readonly,data,contents ${CMAKE_CURRENT_BINARY_DIR}/stdlib.lua.o ${CMAKE_CURRENT_BINARY_DIR}/stdlib.lua.o)

add_library(lua_stdlib
    STATIC
    stdlib.lua.o)

# tell CMake to not compile the file
SET_SOURCE_FILES_PROPERTIES(
    stdlib.lua.o
    PROPERTIES
    EXTERNAL_OBJECT true
    GENERATED true)

# tell CMake to use the C linker
SET_TARGET_PROPERTIES(
    lua_stdlib
    PROPERTIES
    LINKER_LANGUAGE C)

add_library(${PROJECT_NAME} SHARED ${LIB_SRC_LIST})
target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(${PROJECT_NAME}
    PRIVATE TreeSitter
    PRIVATE TreeSitterLua
    PRIVATE lua_stdlib)

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


# documentation
if(DOXYGEN_FOUND)
    set(DOXYGEN_OUTPUT_DIRECTORY ../docs)
    set(DOXYGEN_CLANG_ASSISTED_PARSING YES)
    set(DOXYGEN_CLANG_DATABASE_PATH .)
    set(DOXYGEN_UML_LOOK YES)
    set(DOXYGEN_TEMPLATE_RELATIONS YES)
    set(DOXYGEN_DOT_IMAGE_FORMAT svg)
    set(DOXYGEN_EXTRACT_ALL YES)
    set(DOXYGEN_EXTRACT_PRIVATE YES)
    set(DOXYGEN_EXTRACT_STATIC YES)
    set(DOXYGEN_PLANTUML_JAR_PATH ${PLANTUML_JAR_PATH})
    set(DOXYGEN_EXAMPLE_PATH ${PROJECT_SOURCE_DIR}/tests/docs)

    doxygen_add_docs(${PROJECT_NAME}-docs
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/include)
endif()
