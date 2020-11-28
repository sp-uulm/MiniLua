add_executable(MiniLua-tests
    main.cpp
    unit_tests.cpp
    integration_tests.cpp
    public_api/integration_tests.cpp
    public_api/owning_ptr.cpp
    public_api/values.cpp
    public_api/origin.cpp
    public_api/environment.cpp
    public_api/source_change.cpp
    tree_sitter.cpp)
target_include_directories(MiniLua-tests PRIVATE ${tree-sitter_SOURCE_DIR}/lib/include)
target_link_libraries(MiniLua-tests
    PRIVATE MiniLua
    PRIVATE Catch2::Catch2
    PRIVATE TreeSitter)

if(COVERAGE)
    setup_target_for_coverage(MiniLua-tests-coverage MiniLua-tests coverage)
endif()
