add_executable(MiniLua-bench
    main.cpp
    tree_sitter.cpp)
target_include_directories(MiniLua-bench PRIVATE ${MiniLua_SOURCE_DIR}/src)
target_link_libraries(MiniLua-bench
    PRIVATE Catch2::Catch2
    PRIVATE TreeSitterWrapper
    PRIVATE MiniLua)
