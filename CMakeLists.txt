cmake_minimum_required(VERSION 3.26.0)
project(magma C CXX)

macro(enable_warnings target)
    target_compile_options(${target} PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
    )
endmacro()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build/${CMAKE_BUILD_TYPE}/magma)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build/${CMAKE_BUILD_TYPE}/magma/libs)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build/${CMAKE_BUILD_TYPE}/magma/shared)

if(WIN32)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED TRUE)

if (UNIX AND NOT APPLE)
    option(USE_WAYLAND "If true, will compile with wayland instead of X11" OFF)
endif()

add_subdirectory(${CMAKE_SOURCE_DIR}/mgmcommon)
add_subdirectory(${CMAKE_SOURCE_DIR}/mgmwin)
add_subdirectory(${CMAKE_SOURCE_DIR}/backends)
add_subdirectory(${CMAKE_SOURCE_DIR}/mgmlib)

add_executable(
    magma
        ${CMAKE_CURRENT_SOURCE_DIR}/src/engine.cpp
)

enable_warnings(magma)

get_target_property(MGMCOMMON_INCLUDE_DIRS mgmcommon INTERFACE_INCLUDE_DIRECTORIES)

target_include_directories(
    magma
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIVATE
            ${MGMCOMMON_INCLUDE_DIRS}
)
target_link_libraries(magma PUBLIC mgmcommon mgmwin mgmlib)

add_custom_command(TARGET magma POST_BUILD
    COMMAND cmake -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/mgmcommon/include ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/include
    COMMAND cmake -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/mgmwin/include ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/include
    COMMAND cmake -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/mgmlib/include ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/include
)
