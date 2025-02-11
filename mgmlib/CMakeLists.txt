cmake_minimum_required(VERSION 3.26.0)
project(mgmlib C CXX)

option(EMBED_BACKEND "If true, will embed the selected backend into the executable" OFF)
set(BACKEND_TO_EMBED "none" CACHE STRING "The backend to embed into the executable")

if(UNIX AND NOT APPLE)
    set(
        PLATFORM_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/platform_linux/dloader.cpp
    )
elseif(WIN32)
    set(
        PLATFORM_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/platform_windows/dloader.cpp
    )
endif()

if (EMBED_BACKEND)
    if (BACKEND_TO_EMBED STREQUAL "OpenGL")
        get_target_property(BACKEND_SOURCES backend_OpenGL SOURCES)
        set(BACKEND_EMBED_TARGET backend_OpenGL)
    else()
        message(FATAL_ERROR "Unknown backend to embed: ${BACKEND_TO_EMBED}")
    endif()
endif()

add_library(
    mgmlib
        ${PLATFORM_SOURCES}
        ${BACKEND_SOURCES}
        ${CMAKE_CURRENT_SOURCE_DIR}/src/mgmgpu.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/mgmgpu.hpp
)
enable_warnings(mgmlib)

target_compile_definitions(mgmlib PUBLIC IMGUI_DEFINE_MATH_OPERATORS)

if (EMBED_BACKEND)
    target_compile_definitions(mgmlib PUBLIC EMBED_BACKEND=1)

    get_target_property(BACKEND_INCLUDE_DIRS ${BACKEND_EMBED_TARGET} INCLUDE_DIRECTORIES)
    get_target_property(BACKEND_LIBRARIES ${BACKEND_EMBED_TARGET} LINK_LIBRARIES)
    get_target_property(BACKEND_LINK_OPTIONS ${BACKEND_EMBED_TARGET} LINK_OPTIONS)
    
    target_include_directories(mgmlib PRIVATE ${BACKEND_INCLUDE_DIRS})
    target_link_libraries(mgmlib ${BACKEND_LIBRARIES})
    target_link_options(mgmlib PUBLIC ${BACKEND_LINK_OPTIONS})

    message(STATUS "Embedding backend: ${BACKEND_TO_EMBED}")
endif()

get_target_property(MGMCOMMON_INCLUDE_DIRS mgmcommon INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(MGMWIN_INCLUDE_DIRS mgmwin INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(BACKENDS_INCLUDE_DIR backends INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(
    mgmlib
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/imgui
            ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends
            ${BACKENDS_INCLUDE_DIR}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${MGMCOMMON_INCLUDE_DIRS}
            ${MGMWIN_INCLUDE_DIRS}
)
target_link_libraries(mgmlib mgmcommon backends)

if (UNIX AND NOT APPLE)
    target_link_options(mgmlib PUBLIC -ldl)
endif()
