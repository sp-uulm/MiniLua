# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.18.4)

# We name the project and the target for the ExternalProject_Add() call
# to something that will highlight to the user what we are working on if
# something goes wrong and an error message is produced.

project(tree-sitter-populate NONE)

include(ExternalProject)
ExternalProject_Add(tree-sitter-populate
                     "UPDATE_DISCONNECTED" "False" "GIT_REPOSITORY" "https://github.com/tree-sitter/tree-sitter.git" "GIT_TAG" "0.16.9"
                    SOURCE_DIR          "/home/tim/Documents/Anwendungsprojekt/Bachelor/MiniLua/_deps/tree-sitter-src"
                    BINARY_DIR          "/home/tim/Documents/Anwendungsprojekt/Bachelor/MiniLua/_deps/tree-sitter-build"
                    CONFIGURE_COMMAND   ""
                    BUILD_COMMAND       ""
                    INSTALL_COMMAND     ""
                    TEST_COMMAND        ""
                    USES_TERMINAL_DOWNLOAD  YES
                    USES_TERMINAL_UPDATE    YES
)


