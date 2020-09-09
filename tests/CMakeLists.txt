# DEPENDENCIES
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v2.13.0)
FetchContent_MakeAvailable(Catch2)
FetchContent_MakeAvailable(tree-sitter)

add_executable(MiniLua-tests
    main.cpp
    unit_tests.cpp
    integration_tests.cpp
    tree-sitter.cpp)
target_include_directories(MiniLua-tests PRIVATE ${tree-sitter_SOURCE_DIR}/lib/include)
target_link_libraries(MiniLua-tests
    PRIVATE Catch2::Catch2 MiniLua
    INTERFACE Tree-Sitter)

if(COVERAGE)
    setup_target_for_coverage(MiniLua-tests-coverage MiniLua-tests coverage)
endif()
