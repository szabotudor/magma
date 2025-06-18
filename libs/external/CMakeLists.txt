project(MagmaLibsExternal)


if(MAGMA_USE_GLFW)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/glfw)
endif()
