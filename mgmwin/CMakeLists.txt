cmake_minimum_required(VERSION 3.26.0)
project(mgmwin C CXX)


if(UNIX AND NOT APPLE)
    if (NOT USE_WAYLAND)
        set(PLATFORM_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/platform_x11/mgmwin.cpp
        )
        set(PLATFORM_INCLUDE_DIRS
            ${CMAKE_CURRENT_SOURCE_DIR}/platform_x11
        )
    else()
        set(PLATFORM_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/platform_wayland/mgmwin.cpp
        )
    endif()
elseif(WIN32)
    set(PLATFORM_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/platform_windows/mgmwin.cpp
    )
    set(PLATFORM_INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/platform_windows
    )
endif()

add_library(
    mgmwin
        ${PLATFORM_SOURCES}

        ${CMAKE_CURRENT_SOURCE_DIR}/include/mgmwin.hpp
)
enable_warnings(mgmwin)

get_target_property(MGMCOMMON_INCLUDE_DIRS mgmcommon INTERFACE_INCLUDE_DIRECTORIES)

target_include_directories(
    mgmwin
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIVATE
            ${MGMCOMMON_INCLUDE_DIRS}
            ${PLATFORM_INCLUDE_DIRS}
)
if (UNIX AND NOT APPLE)
    target_link_options(mgmwin PUBLIC -lX11)
endif()
