project(MagmaEngine)


add_library(
    MagmaEngine STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include/magma.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/mgm_window.hpp

        ${CMAKE_CURRENT_SOURCE_DIR}/src/magma.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/mgm_window.cpp
)

target_link_libraries(MagmaEngine PUBLIC mgmath MagmaStandardLibs)

if(MAGMA_USE_GLFW)
    target_link_libraries(MagmaEngine PUBLIC glfw)
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
