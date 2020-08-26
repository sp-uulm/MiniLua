# DEPENDENCIES
FetchContent_Declare(
    catch
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v2.13.0)
FetchContent_MakeAvailable(catch)

# TESTS
add_executable(unit_tests unit_tests.cpp)
target_link_libraries(unit_tests PRIVATE Catch2::Catch2)
add_test(NAME test_unit_tests COMMAND unit_tests)

