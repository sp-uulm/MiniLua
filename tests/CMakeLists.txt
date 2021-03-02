add_executable(MiniLua-tests
    main.cpp
    unit_tests.cpp
    integration_tests.cpp
    public_api/integration_tests.cpp
    public_api/owning_ptr.cpp
    public_api/values.cpp
    public_api/origin.cpp
    public_api/environment.cpp
    stdlib_tests.cpp
    math_tests.cpp
    tree_sitter_ast_tests.cpp
    gc.cpp)
target_include_directories(MiniLua-tests PRIVATE ${tree-sitter_SOURCE_DIR}/lib/include)
# add include directory to be able to test internal headers
target_include_directories(MiniLua-tests PRIVATE ${MiniLua_SOURCE_DIR}/src)
target_link_libraries(MiniLua-tests
        PRIVATE MiniLua
        PRIVATE Catch2::Catch2
        PRIVATE TreeSitterWrapper)

if(COVERAGE)
    setup_target_for_coverage(MiniLua-tests-coverage MiniLua-tests coverage)
endif()

add_subdirectory(docs)
