cmake_minimum_required(VERSION 3.26.0)
project(mgmlib C CXX)

if(UNIX AND NOT APPLE)
    set(
        PLATFORM_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/platform_linux/dloader.cpp
    )
endif()

add_library(
    mgmlib 
        ${CMAKE_CURRENT_SOURCE_DIR}/src/mgmlib.cpp
        ${PLATFORM_SOURCES}
)

get_target_property(MGMCOMMON_INCLUDE_DIRS mgmcommon INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(MGMWIN_INCLUDE_DIRS mgmwin INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(
    mgmlib
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${MGMCOMMON_INCLUDE_DIRS}
            ${MGMWIN_INCLUDE_DIRS}
)
target_link_libraries(mgmlib mgmcommon)
