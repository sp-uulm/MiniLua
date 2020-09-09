# DEPENDENCIES
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v2.13.0)
FetchContent_MakeAvailable(Catch2)
FetchContent_MakeAvailable(tree-sitter)

add_executable(MiniLua-bench
    main.cpp
    tree_sitter.cpp)
target_include_directories(MiniLua-bench PRIVATE ${tree-sitter_SOURCE_DIR}/lib/include)
target_link_libraries(MiniLua-bench
    PRIVATE Catch2::Catch2 MiniLua)
