cmake_minimum_required(VERSION 3.5)

#project(MiniluaGui2 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)

set(PROJECT_SOURCES
        main.cpp
        minilua.cpp
        minilua.hpp
        minilua.ui
)
set(PROJECT_NAME_GUI
    ${PROJECT_NAME}-newgui
)
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    add_executable(
        ${PROJECT_NAME_GUI}
        ${PROJECT_SOURCES})
    target_include_directories(${PROJECT_NAME_GUI} PRIVATE . ${PROJECT_NAME})

    target_link_libraries(${PROJECT_NAME_GUI}
            PRIVATE Qt::Core Qt::Widgets Qt::Gui ${PROJECT_NAME})
else()
    message(WARN "Qt6 was not found. Disabling gui example")    
endif()
