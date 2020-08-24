# core
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/core CORE_SRC_LIST)
add_library(${PROJECT_NAME} SHARED ${CORE_SRC_LIST})
target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:../include>
)

# needed for installation
file(GLOB HEADER_LIST CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/include/*.h")
set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "${HEADER_LIST}")

install(TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}
        LIBRARY)
install(EXPORT ${PROJECT_NAME}
        FILE ${PROJECT_NAME}Config.cmake
        DESTINATION lib/cmake/${PROJECT_NAME})
