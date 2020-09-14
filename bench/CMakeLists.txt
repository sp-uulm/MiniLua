add_executable(MiniLua-bench
    main.cpp
    tree_sitter.cpp)
target_include_directories(MiniLua-bench PRIVATE ${tree-sitter_SOURCE_DIR}/lib/include)
target_link_libraries(MiniLua-bench
    PRIVATE Catch2::Catch2
    PRIVATE TreeSitter
    PRIVATE MiniLua)
