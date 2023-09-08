cmake_minimum_required(VERSION 3.26.0)
project(mgmcommon C CXX)
set(MGMCOMMON_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "mgmcommon library path")


if(UNIX)
    list(APPEND platform_sources
        ${MGMCOMMON_SOURCE_DIR}/platform/platform_unix.cpp
    )
elseif(WIN32)
    list(APPEND platform_sources
        ${MGMCOMMON_SOURCE_DIR}/platform/platform_windows.cpp
    )
endif()

add_library(
    mgmcommon
        ${MGMCOMMON_SOURCE_DIR}/src/logging.cpp
        ${platform_sources}
)

target_include_directories(
    mgmcommon
        PUBLIC
            ${MGMCOMMON_SOURCE_DIR}/include
)

if (UNIX AND NOT APPLE)
    target_compile_options(mgmcommon PUBLIC -fPIC)
endif()
