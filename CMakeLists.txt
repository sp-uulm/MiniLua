# set minimum version of CMake
# (this is compatible with all version of CMake but versions before 3.12 ignore
# the range and only use the first version)
cmake_minimum_required(VERSION 3.11...3.18)
# fallback to provide the same functionallity as in CMake 3.12+
if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

# project definition
project(MiniLua VERSION 1.0
                DESCRIPTION "Value tracing lua interpreter"
                LANGUAGES CXX C)

# global settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_compile_options(-Wall)

# generates compile_commands.json that can be used by clang tooling
# (e.g. language server)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")

message(INFO ${CMAKE_MODULE_PATH})

include(FetchContent)
include(add_FetchContent_MakeAvailable)
find_package(Sanitizers)

set(COVERAGE OFF CACHE BOOL "Turns on code coverage generation for tests")
if(COVERAGE)
    include(CodeCoverage)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_COVERAGE}")
    # remove tests and dependencies from coverage report
    set(LCOV_REMOVE_EXTRA "'${CMAKE_SOURCE_DIR}/tests/*'" "'${CMAKE_BINARY_DIR}/*'")
endif()

# library
add_subdirectory(src)

# example applications
# (get compiled in ci but not run like tests)
add_subdirectory(examples)

# tests
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    add_subdirectory(tests)
    add_subdirectory(bench)
endif()
