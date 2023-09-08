cmake_minimum_required(VERSION 3.26.0)
project(mgmcommon C CXX)
set(MGMCOMMON_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "mgmcommon library path")


add_library(
    mgmcommon
        ${MGMCOMMON_SOURCE_DIR}/src/file.cpp
        ${MGMCOMMON_SOURCE_DIR}/src/logging.cpp
)

target_include_directories(
    mgmcommon
        PUBLIC
            ${MGMCOMMON_SOURCE_DIR}/include
            ${MGMCOMMON_SOURCE_DIR}/platform
)

if (UNIX AND NOT APPLE)
    target_compile_options(mgmcommon PUBLIC -fPIC)
endif()
