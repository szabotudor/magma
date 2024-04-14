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
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_mgmgpu.cpp
)
set(
    IMGUI_HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imconfig.h
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.h
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_internal.h
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imstb_rectpack.h
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imstb_textedit.h
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imstb_truetype.h
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_mgmgpu.h
)

add_library(
    mgmlib
        ${IMGUI_SOURCES}
        ${IMGUI_HEADERS}
        ${PLATFORM_SOURCES}
        ${CMAKE_CURRENT_SOURCE_DIR}/src/mgmgpu.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/mgmgpu.hpp
)
enable_warnings(mgmlib)

get_target_property(MGMCOMMON_INCLUDE_DIRS mgmcommon INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(MGMWIN_INCLUDE_DIRS mgmwin INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(BACKENDS_INCLUDE_DIR backends INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(
    mgmlib
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/imgui
            ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends
            ${BACKENDS_INCLUDE_DIR}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${MGMCOMMON_INCLUDE_DIRS}
            ${MGMWIN_INCLUDE_DIRS}
)
target_link_libraries(mgmlib mgmcommon)
target_link_options(mgmlib PUBLIC -ldl)
