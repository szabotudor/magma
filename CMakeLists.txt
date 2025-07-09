cmake_minimum_required(VERSION 4.0)
project(Magma)


# C/C++ config
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED TRUE)

set(CMAKE_CXX_SCAN_FOR_MODULES FALSE)


# Setup directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin/libs)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_BINARY_DIR}/bin/libs)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR}/bin/libs)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin/shared)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_BINARY_DIR}/bin/shared)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR}/bin/shared)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR}/bin)

set(CMAKE_ROOT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})


if (${CMAKE_BUILD_TYPE} STREQUAL "Release")
    add_compile_definitions(MAGMA_RELEASE)
    message(STATUS "Compiling in RELEASE")
elseif(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    add_compile_definitions(MAGMA_DEBUG)
    message(STATUS "Compiling in DEBUG")
endif()


# Platform specific config
set(POSSIBLE_BACKENDS)

if(WIN32 AND MSVC)
    message(FATAL_ERROR "MSVC is not supported")    
endif()

if(WIN32 OR (UNIX AND NOT APPLE))
    set(MAGMA_USE_GLFW TRUE)
    list(APPEND POSSIBLE_BACKENDS "OpenGL" "Vulkan")

    if (WIN32)
        list(APPEND POSSIBLE_BACKENDS "DirectX12")
    endif()
endif()

message(STATUS "Possible backends: ${POSSIBLE_BACKENDS}")

set(GraphicsBackend "-" CACHE STRING "Chose graphics API to use for the backend")
set_property(CACHE GraphicsBackend PROPERTY STRINGS "-" ${POSSIBLE_BACKENDS})

if (GraphicsBackend STREQUAL "-")
    message(FATAL_ERROR "No graphics backend selected")
elseif (NOT GraphicsBackend IN_LIST POSSIBLE_BACKENDS)
    message(FATAL_ERROR "Please select a valid backend from the list: ${POSSIBLE_BACKENDS} (Current backend: ${GraphicsBackend})")
else()
    message(STATUS "Using graphics backend: ${GraphicsBackend}")
    add_compile_definitions(MAGMA_BACKEND_${GraphicsBackend})
endif()


# Libs
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/libs/external)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/libs/internal)


# Engine modules
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/engine)


# Editor executable
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/editor)
