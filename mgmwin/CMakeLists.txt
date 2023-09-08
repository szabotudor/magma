cmake_minimum_required(VERSION 3.26.0)
project(mgmwin C CXX)
set(MGMWIN_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "mgmwin library path")


if(UNIX AND NOT APPLE)
    if (NOT USE_WAYLAND)
        list(APPEND platform_sources
            ${MGMWIN_SOURCE_DIR}/platform/platform_x11.cpp
        )
    else()
        list(APPEND platform_sources
            ${MGMWIN_SOURCE_DIR}/platform/platform_wayland.cpp
        )
    endif()
elseif(WIN32)
    list(APPEND platform_sources
        ${MGMWIN_SOURCE_DIR}/platform/platform_windows.cpp
    )
endif()

add_library(
    mgmwin
        ${platform_sources}
)

target_include_directories(
    mgmwin
        PUBLIC
            ${MGMWIN_SOURCE_DIR}/include
        PRIVATE
            ${MGMCOMMON_SOURCE_DIR}/include
)
if (UNIX AND NOT APPLE)
    target_link_options(mgmwin PUBLIC -lX11)
endif()
