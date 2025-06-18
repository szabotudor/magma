project(MagmaEditor)


add_executable(
    MagmaEditor
        ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    
        ${CMAKE_CURRENT_SOURCE_DIR}/include/editor.hpp

        ${CMAKE_CURRENT_SOURCE_DIR}/src/editor.cpp
)


target_link_libraries(MagmaEditor PUBLIC MagmaEngine)
