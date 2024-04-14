cmake_minimum_required(VERSION 3.26.0)
project(backends C CXX)


if(UNIX)
    list(APPEND PLATFORM_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/backend_OpenGL/platform_unix/backend_OpenGL.cpp
    )
elseif(WIN32)
    list(APPEND PLATFORM_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/backend_OpenGL/platform_windows/backend_OpenGL.cpp
    )
endif()

add_library(backends INTERFACE)
target_include_directories(
    backends
        INTERFACE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_library(
    backend_OpenGL MODULE
        ${CMAKE_CURRENT_SOURCE_DIR}/backend_OpenGL/backend_OpenGL.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/backend_OpenGL/glad/src/glad.c
        ${PLATFORM_SOURCES}
        ${CMAKE_CURRENT_SOURCE_DIR}/include/backend_settings.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/backend_OpenGL/include/backend_OpenGL.hpp
)
enable_warnings(backend_OpenGL)

get_target_property(MGMCOMMON_INCLUDE_DIRS mgmcommon INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(MGMWIN_INCLUDE_DIRS mgmwin INCLUDE_DIRECTORIES)
target_include_directories(
    backend_OpenGL
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/backend_OpenGL/glad/include
            ${CMAKE_CURRENT_SOURCE_DIR}/backend_OpenGL/include
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${MGMCOMMON_INCLUDE_DIRS}
        PRIVATE
            ${MGMWIN_INCLUDE_DIRS}
)
target_link_libraries(backend_OpenGL mgmcommon)

if(UNIX AND NOT APPLE)
    target_link_options(backend_OpenGL PUBLIC -lGL -lEGL)
elseif(WIN32)
    find_package(OpenGL REQUIRED)
    target_link_libraries(backend_OpenGL opengl32)
endif()
