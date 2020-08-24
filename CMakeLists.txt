# set minimum version of CMake
# (this is compatible with all version of CMake but versions before 3.12 ignore
# the range and only use the first version)
cmake_minimum_required(VERSION 3.8...3.18)
# fallback to provide the same functionallity as in CMake 3.12+
if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

# project definition
project(MiniLua VERSION 1.0
                DESCRIPTION "Value tracing lua interpreter"
                LANGUAGES CXX)

# global settings
set(CMAKE_CXX_STANDARD 17)
add_compile_options(-Wall)

# dependencies
find_package(Qt5 COMPONENTS Core Widgets REQUIRED)
find_package(Boost REQUIRED)

# library
add_subdirectory(src)

# example applications
add_subdirectory(examples)

