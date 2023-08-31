cmake_minimum_required(VERSION 3.26.0)
project(mgmwin C CXX)
set(MGMWIN_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "mgmwin library path")


add_library(
    mgmwin 
        ${MGMWIN_SOURCE_DIR}/src/mgmwin.cpp
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