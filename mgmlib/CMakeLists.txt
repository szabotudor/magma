cmake_minimum_required(VERSION 3.26.0)
project(mgmlib C CXX)
set(MGMLIB_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "mgmlib library path")
list(APPEND CMAKE_MODULE_PATH ${MGMLIB_SOURCE_DIR})



add_library(
    mgmlib 
        ${MGMLIB_SOURCE_DIR}/src/mgmlib.cpp
        ${MGMLIB_SOURCE_DIR}/src/dloader.cpp
)

target_include_directories(
    mgmlib
        PUBLIC
            ${MGMLIB_SOURCE_DIR}/include
        PRIVATE
            ${MGMLIB_SOURCE_DIR}/src
            ${MGMCOMMON_SOURCE_DIR}
            ${MGMWIN_SOURCE_DIR}/include
)
target_link_libraries(mgmlib mgmcommon)


add_library(
    backend_OpenGL MODULE
        ${MGMLIB_SOURCE_DIR}/src/backend_OpenGL.cpp
        ${MGMLIB_SOURCE_DIR}/glad/src/glad.c
)
target_include_directories(
    backend_OpenGL
        PUBLIC
            ${MGMLIB_SOURCE_DIR}/include
            ${MGMLIB_SOURCE_DIR}/glad/include
            ${MGMCOMMON_SOURCE_DIR}/include
            ${MGMWIN_SOURCE_DIR}/include
)
target_link_libraries(backend_OpenGL mgmcommon)

if(UNIX AND NOT APPLE)
    target_link_options(backend_OpenGL PUBLIC -lGL -lEGL)
elseif(WIN32)
    find_package(OpenGL REQUIRED)
    target_link_libraries(backend_OpenGL opengl32)
endif()
