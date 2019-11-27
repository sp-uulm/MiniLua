project(MiniLua)
cmake_minimum_required(VERSION 3.0)

#set(CMAKE_VERBOSE_MAKEFILE ON)

aux_source_directory(src/core CORE_SRC_LIST)
aux_source_directory(src/gui GUI_SRC_LIST)
aux_source_directory(src/cli CLI_SRC_LIST)

find_package(Qt5 COMPONENTS Core Widgets REQUIRED)
find_package(Boost REQUIRED)

#set(CMAKE_AUTOMOC ON)

add_compile_options(-std=c++17 -Wall)
set(CMAKE_CXX_STANDARD 17)

include_directories(include)

add_library(${PROJECT_NAME} SHARED ${CORE_SRC_LIST})
set_target_properties(${PROJECT_NAME} PROPERTIES
  PUBLIC_HEADER "include/luaparser.h;include/luainterpreter.h;include/luaast.h"
)

qt5_wrap_cpp(gui_SRC include/gui.h)
add_executable(${PROJECT_NAME}-gui ${GUI_SRC_LIST} ${gui_SRC})

target_include_directories(${PROJECT_NAME} PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
                                                  "$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>")

target_link_libraries(${PROJECT_NAME}-gui ${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME}-gui Qt5::Widgets Qt5::Core)

add_executable(${PROJECT_NAME}-cli ${CLI_SRC_LIST})
target_link_libraries(${PROJECT_NAME}-cli ${PROJECT_NAME})

install(TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}
        LIBRARY DESTINATION lib
        PUBLIC_HEADER DESTINATION include)
install(TARGETS ${PROJECT_NAME}-gui ${PROJECT_NAME}-cli RUNTIME DESTINATION bin)
install(EXPORT ${PROJECT_NAME}
        FILE ${PROJECT_NAME}Config.cmake        
        DESTINATION lib/cmake/${PROJECT_NAME})

