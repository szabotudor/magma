cmake_minimum_required(VERSION 3.26.0)
project(magma C CXX)

macro(enable_warnings target)
    target_compile_options(${target} PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wno-cpp -Wno-non-template-friend -Wall -Wextra -Wpedantic -Wshadow -Wcast-align -Wredundant-decls -Wconversion -Wformat=2 -Wunreachable-code -Wcast-qual -Wwrite-strings -Winit-self -Wlogical-op -Wno-unknown-pragmas -Wmissing-field-initializers -Wmissing-noreturn -Wduplicated-branches -Wodr -Wdouble-promotion -Wno-multichar -Wnull-dereference -Wformat-overflow=2 -Walloc-zero -Walloca>
    )
endmacro()

macro(ignore_warnings_for_files)
    foreach(file ${ARGN})
        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            # GCC
            set_source_files_properties(${file} PROPERTIES COMPILE_FLAGS "-w")
        elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
            # Clang
            set_source_files_properties(${file} PROPERTIES COMPILE_FLAGS "-w")
        elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
            # MSVC
            set_source_files_properties(${file} PROPERTIES COMPILE_FLAGS "/w")
        endif()
    endforeach()
endmacro()

# add_compile_definitions(MGMATH_SIMD=1 MGMATH_SWIZZLE=1)
add_compile_definitions(MGMATH_SWIZZLE=1)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}/magma)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}/magma/libs)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}/magma/shared)

if(WIN32)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED TRUE)

if (UNIX AND NOT APPLE)
    option(USE_WAYLAND "If true, will compile with wayland instead of X11" OFF)
endif()

option(ENABLE_EDITOR "If true, will compile the editor" ON)

#add_subdirectory(${CMAKE_SOURCE_DIR}/luajit)
add_subdirectory(${CMAKE_SOURCE_DIR}/mgmcommon)
add_subdirectory(${CMAKE_SOURCE_DIR}/mgmwin)
add_subdirectory(${CMAKE_SOURCE_DIR}/backends)
add_subdirectory(${CMAKE_SOURCE_DIR}/mgmlib)

if (ENABLE_EDITOR)
    set(
        EDITOR_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/src/systems/editor.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/editor_windows/editor_palette.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/editor_windows/file_browser.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/editor_windows/script_editor.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/editor_windows/settings.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/editor_windows/scene_view.cpp

            ${CMAKE_CURRENT_SOURCE_DIR}/include/systems/editor.hpp
            ${CMAKE_CURRENT_SOURCE_DIR}/include/editor_windows/file_browser.hpp
            ${CMAKE_CURRENT_SOURCE_DIR}/include/editor_windows/script_editor.hpp
            ${CMAKE_CURRENT_SOURCE_DIR}/include/editor_windows/settings.hpp
            ${CMAKE_CURRENT_SOURCE_DIR}/include/editor_windows/scene_view.hpp
    )
endif()

add_library(
    magma STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/engine.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/systems.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/systems/input.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/systems/notifications.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/systems/resources.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/systems/renderer.cpp
        #${CMAKE_CURRENT_SOURCE_DIR}/src/tools/lua_vm.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/built-in_components/renderable.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ecs.cpp

        ${CMAKE_CURRENT_SOURCE_DIR}/include/engine.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/tools/expose_api.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/systems/input.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/systems/notifications.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/systems/resources.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/systems/renderer.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/systems.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/tools/base64.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/tools/tiny_obj_loader.h
        #${CMAKE_CURRENT_SOURCE_DIR}/include/tools/lua_vm.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/tools/mgmecs.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/built-in_components/renderable.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/ecs.hpp

        ${EDITOR_SOURCES}
)

if (ENABLE_EDITOR)
    target_compile_definitions(magma PRIVATE ENABLE_EDITOR)
    message(STATUS "Editor enabled")
endif()

enable_warnings(magma)

get_target_property(MGMCOMMON_INCLUDE_DIRS mgmcommon INTERFACE_INCLUDE_DIRECTORIES)
#get_target_property(LUAJIT_INCLUDE_DIRS luajit INTERFACE_INCLUDE_DIRECTORIES)

target_include_directories(
    magma
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIVATE
            ${MGMCOMMON_INCLUDE_DIRS}
            #${LUAJIT_INCLUDE_DIRS}
)
target_link_libraries(magma PUBLIC mgmcommon mgmwin mgmlib)

add_custom_command(TARGET magma POST_BUILD
    COMMAND cmake -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/mgmcommon/include ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/include
    COMMAND cmake -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/mgmwin/include ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/include
    COMMAND cmake -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/mgmlib/include ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/include
    COMMAND cmake -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/include
    COMMAND cmake -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/resources ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources
)

add_executable(
    magma_engine
        ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)
enable_warnings(magma_engine)

if (CMAKE_BUILD_TYPE STREQUAL "Debug" AND UNIX AND NOT APPLE)
    target_compile_options(mgmcommon PRIVATE -O0)
    target_compile_options(mgmwin PRIVATE -O0)
    target_compile_options(mgmlib PRIVATE -O0)
    target_compile_options(magma PRIVATE -O0)
    target_compile_options(magma_engine PRIVATE -O0)
endif()

target_include_directories(
    magma_engine
        PRIVATE
            ${MGMCOMMON_INCLUDE_DIRS}
            ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(magma_engine PRIVATE magma)
