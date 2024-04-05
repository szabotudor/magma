cmake_minimum_required(VERSION 3.26.0)
project(mgmcommon C CXX)


if(UNIX AND NOT APPLE)
    set(
        PLATFORM_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/platform_linux/file.cpp
    )
elseif(WIN32)
    set(
        PLATFORM_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/platform_windows/file.cpp
    )
endif()

if(NOT STDIO_COMPATIBLE)
    list(APPEND
        PLATFORM_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/platform_stdio_compatible/logging.cpp
        )
endif()

add_library(
    mgmcommon
        ${CMAKE_CURRENT_SOURCE_DIR}/src/logging.cpp
        ${PLATFORM_SOURCES}
)
enable_warnings(mgmcommon)

target_include_directories(
    mgmcommon
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if (UNIX AND NOT APPLE)
    target_compile_options(mgmcommon PUBLIC -fPIC)
endif()
