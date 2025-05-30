cmake_minimum_required(VERSION 3.26.0)
project(mgmcommon C CXX)


if(UNIX AND NOT APPLE)
    set(
        PLATFORM_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/platform_linux/file.cpp
    )
    set(STDIO_COMPATIBLE ON)
    set(FILESYSTEM_COMPATIBLE ON)
elseif(WIN32)
    set(
        PLATFORM_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/platform_windows/file.cpp
    )
    set(STDIO_COMPATIBLE ON)
    set(FILESYSTEM_COMPATIBLE ON)
endif()

if(STDIO_COMPATIBLE)
    list(APPEND
        PLATFORM_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/platform_stdio_compatible/logging.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/platform_stdio_compatible/file.cpp
        )
endif()
if(FILESYSTEM_COMPATIBLE)
    list(APPEND
        PLATFORM_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/platform_filesystem_compatible/file.cpp
        )
endif()

add_library(
    mgmcommon
        ${CMAKE_CURRENT_SOURCE_DIR}/src/file.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/helpers.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/logging.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/json.cpp
        ${PLATFORM_SOURCES}

        ${CMAKE_CURRENT_SOURCE_DIR}/include/file.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/helpers.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/json.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/logging.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/mgmath/mgmath.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/types.hpp
)
enable_warnings(mgmcommon)

ignore_warnings_for_files(${CMAKE_CURRENT_SOURCE_DIR}/include/helpers.hpp)

target_include_directories(
    mgmcommon
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/include/mgmath
)

if (UNIX AND NOT APPLE)
    target_compile_options(mgmcommon PUBLIC -fPIC)
endif()
