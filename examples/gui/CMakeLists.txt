find_package(Qt5 COMPONENTS Core Widgets REQUIRED)

qt5_wrap_cpp(gui_SRC gui.h)
aux_source_directory("${CMAKE_CURRENT_SOURCE_DIR}" GUI_SRC_LIST)
add_executable(${PROJECT_NAME}-gui ${GUI_SRC_LIST} ${gui_SRC})
target_link_libraries(${PROJECT_NAME}-gui PUBLIC ${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME}-gui PUBLIC Qt5::Widgets Qt5::Core)

install(TARGETS ${PROJECT_NAME}-gui RUNTIME DESTINATION bin)
