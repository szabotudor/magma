cmake_minimum_required(VERSION 3.26.0)
project(mgmlib C CXX)

if(UNIX AND NOT APPLE)
    set(
        PLATFORM_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/platform_linux/dloader.cpp
    )
elseif(WIN32)
    set(
        PLATFORM_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/platform_windows/dloader.cpp
    )
endif()

set(
    IMGUI_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_draw.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_tables.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_widgets.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_mgmgfx.cpp
)

add_library(
    mgmlib 
        ${CMAKE_CURRENT_SOURCE_DIR}/src/mgmgfx.cpp
        ${IMGUI_SOURCES}
        ${PLATFORM_SOURCES}
)

get_target_property(MGMCOMMON_INCLUDE_DIRS mgmcommon INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(MGMWIN_INCLUDE_DIRS mgmwin INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(
    mgmlib
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/imgui
            ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${MGMCOMMON_INCLUDE_DIRS}
            ${MGMWIN_INCLUDE_DIRS}
)
target_link_libraries(mgmlib mgmcommon)
