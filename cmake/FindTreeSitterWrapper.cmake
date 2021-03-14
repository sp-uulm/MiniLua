# Finds the Tree-Sitter C++ Wrapper library
#
# Adds the following imported targets
#
#   TreeSitterWrapper

find_path(TreeSitterWrapper_SOURCE_DIR
    NAMES CMakeLists.txt
    PATHS "${CMAKE_CURRENT_SOURCE_DIR}/extern/tree-sitter-cpp-api"
    NO_DEFAULT_PATH)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(TreeSitterWrapper
    REQUIRED_VARS TreeSitterWrapper_SOURCE_DIR)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/extern/tree-sitter-cpp-api)
