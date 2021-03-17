set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Core Widgets Gui)

if (NOT Qt6_FOUND)
    message(WARN "Qt6 was not found. Disabling gui example")
else()
    add_executable(${PROJECT_NAME}-newgui
        # mainwindow.ui
        mainwindow.cpp
        main.cpp)
        # resources.qrc)
    target_include_directories(${PROJECT_NAME}-newgui PRIVATE .)

    target_link_libraries(${PROJECT_NAME}-newgui
        PRIVATE Qt::Core Qt::Widgets Qt::Gui ${PROJECT_NAME})
endif()
