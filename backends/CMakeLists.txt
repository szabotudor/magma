cmake_minimum_required(VERSION 3.26.0)
project(backends C CXX)
set(BACKENDS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "backends library path")
list(APPEND CMAKE_MODULE_PATH ${BACKENDS_SOURCE_DIR})


if(UNIX)
    list(APPEND platform_sources
        ${BACKENDS_SOURCE_DIR}/backend_OpenGL/platform/platform_unix.cpp
    )
elseif(WIN32)
    list(APPEND platform_sources
        ${BACKENDS_SOURCE_DIR}/backend_OpenGL/platform/platform_windows.cpp
    )
endif()

add_library(
    backend_OpenGL MODULE
        ${BACKENDS_SOURCE_DIR}/backend_OpenGL/backend_OpenGL.cpp
        ${BACKENDS_SOURCE_DIR}/backend_OpenGL/glad/src/glad.c
        ${platform_sources}
)
target_include_directories(
    backend_OpenGL
        PUBLIC
            ${BACKENDS_SOURCE_DIR}/backend_OpenGL/glad/include
            ${MGMCOMMON_SOURCE_DIR}/include
            ${MGMWIN_SOURCE_DIR}/include
        PRIVATE
            ${BACKENDS_SOURCE_DIR}/backend_OpenGL/platform
)
target_link_libraries(backend_OpenGL mgmcommon)

if(UNIX AND NOT APPLE)
    target_link_options(backend_OpenGL PUBLIC -lGL -lEGL)
elseif(WIN32)
    find_package(OpenGL REQUIRED)
    target_link_libraries(backend_OpenGL opengl32)
endif()
