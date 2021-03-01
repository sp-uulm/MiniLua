# NOTE: This "executable" does nothing it basically only ensures that the code
# in the documentation compiles.

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} DOCTEST_SOURCES)

add_executable(MiniLua-doc-tests ${DOCTEST_SOURCES})
target_include_directories(MiniLua-doc-tests PRIVATE ${MiniLua_SOURCE_DIR}/src)
target_link_libraries(MiniLua-doc-tests
        PRIVATE MiniLua)
        # PRIVATE Catch2::Catch2
        # PRIVATE TreeSitter)
