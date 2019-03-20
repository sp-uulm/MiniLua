project(MiniLua)
cmake_minimum_required(VERSION 3.0)
aux_source_directory(. SRC_LIST)

find_package(Qt5 COMPONENTS Core Widgets REQUIRED)

#set(CMAKE_AUTOMOC ON)

add_compile_options(-std=c++17 -Wall)
set(CMAKE_CXX_STANDARD 17)

qt5_wrap_cpp(gui_SRC include/gui.h)
add_executable(${PROJECT_NAME} ${SRC_LIST} ${gui_SRC})

#target_include_directories(${PROJECT_NAME} PUBLIC include)
target_link_libraries(${PROJECT_NAME} Qt5::Widgets Qt5::Core)
