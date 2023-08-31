cmake_minimum_required(VERSION 3.26.0)
project(magma C CXX)
set(MAGMA_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${MAGMA_SOURCE_DIR}/build/${CMAKE_CXX_PLATFORM_ID}/${CMAKE_BUILD_TYPE})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${MAGMA_SOURCE_DIR}/build/${CMAKE_CXX_PLATFORM_ID}/${CMAKE_BUILD_TYPE}/libs)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${MAGMA_SOURCE_DIR}/build/${CMAKE_CXX_PLATFORM_ID}/${CMAKE_BUILD_TYPE}/shared)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED TRUE)

add_subdirectory(${CMAKE_SOURCE_DIR}/mgmcommon)
add_subdirectory(${CMAKE_SOURCE_DIR}/mgmwin)
add_subdirectory(${CMAKE_SOURCE_DIR}/mgmlib)

add_executable(
    magma
        ${MAGMA_SOURCE_DIR}/src/engine.cpp
)

target_include_directories(
    magma
        PUBLIC
            ${MAGMA_SOURCE_DIR}/include
        PRIVATE
            ${MGMCOMMON_SOURCE_DIR}/include
)
target_link_libraries(magma PUBLIC mgmcommon mgmwin mgmlib)

add_custom_command(TARGET magma POST_BUILD
    COMMAND cmake -E copy_directory ${MAGMA_SOURCE_DIR}/mgmcommon/include ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/include
    COMMAND cmake -E copy_directory ${MAGMA_SOURCE_DIR}/mgmwin/include ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/include
    COMMAND cmake -E copy_directory ${MAGMA_SOURCE_DIR}/mgmlib/include ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/include
)
