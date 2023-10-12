cmake_minimum_required(VERSION 3.26.0)
project(mgmwin C CXX)


if(UNIX AND NOT APPLE)
    if (NOT USE_WAYLAND)
        set(PLATFORM_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/platform_x11/mgmwin.cpp
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
endif()

add_library(
    mgmwin
        ${PLATFORM_SOURCES}
)

get_target_property(MGMCOMMON_INCLUDE_DIRS mgmcommon INTERFACE_INCLUDE_DIRECTORIES)

target_include_directories(
    mgmwin
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIVATE
            ${MGMCOMMON_INCLUDE_DIRS}
)
if (UNIX AND NOT APPLE)
    target_link_options(mgmwin PUBLIC -lX11)
endif()
