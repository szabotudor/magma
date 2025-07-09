project(MagmaEngine)


if (MAGMA_USE_GLFW)
    set(PLATFORM_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/mgm_window_glfw.cpp)
endif()

if (GraphicsBackend STREQUAL "OpenGL")
    list(APPEND PLATFORM_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/mgm_backend_opengl.cpp)
endif()

add_library(
    MagmaEngine STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include/magma.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/mgm_window.hpp

        ${CMAKE_CURRENT_SOURCE_DIR}/src/magma.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/mgm_window.cpp

        ${PLATFORM_SOURCES}
)

target_link_libraries(MagmaEngine PUBLIC mgmath MagmaStandardLibs)

if(MAGMA_USE_GLFW)
    target_link_libraries(MagmaEngine PUBLIC glfw)
endif()

message(STATUS ${GraphicsBackend})

if (GraphicsBackend STREQUAL "OpenGL")
    target_link_libraries(MagmaEngine PUBLIC glad)
    message(STATUS "Linking glad for OpenGL")
endif()

target_include_directories(
    MagmaEngine
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/include
)


add_custom_command(
    TARGET MagmaEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_ROOT_SOURCE_DIR}/engine ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/engine
        BYPRODUCTS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/engine
)

add_custom_command(
    TARGET MagmaEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_ROOT_SOURCE_DIR}/libs ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/libs
        BYPRODUCTS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/libs/internal ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/libs/external
)
