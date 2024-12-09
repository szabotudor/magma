cmake_minimum_required(VERSION 3.26.0)
project(luajit_project)

include(ExternalProject)


set(LUAJIT_MAKE_CMD "")
set(LUAJIT_LIB_PATH "")

if (UNIX AND NOT APPLE)
    set(LUAJIT_MAKE_CMD "make")
    set(LUAJIT_LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src/libluajit.a)
elseif(WIN32)
    if (MSVC)
        set(LUAJIT_MAKE_CMD "nmake")
        set(LUAJIT_LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src/luajit.lib)
    elseif(MINGW)
        set(LUAJIT_MAKE_CMD "mingw32-make")
        set(LUAJIT_LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src/libluajit.a)
    else()
        message(FATAL_ERROR "Unsupported compiler on Windows")
    endif()
else()
    message(FATAL_ERROR "Unsupported platform")
endif()


ExternalProject_Add(luajit_project
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ${LUAJIT_MAKE_CMD}
    BUILD_IN_SOURCE TRUE
    BUILD_BYPRODUCTS ${LUAJIT_LIB_PATH}
    INSTALL_COMMAND ""
)


add_library(luajit STATIC IMPORTED GLOBAL)

message(STATUS "Lua binaries: ${LUAJIT_LIB_PATH}")

set_target_properties(
    luajit
    PROPERTIES
        IMPORTED_LOCATION ${LUAJIT_LIB_PATH}
)

target_include_directories(
    luajit
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
