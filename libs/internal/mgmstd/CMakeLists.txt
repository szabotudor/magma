project(MagmaStandardLibs)

add_library(
    MagmaStandardLibs
        STATIC
            ${CMAKE_CURRENT_SOURCE_DIR}/mgm_allocator.hpp
            ${CMAKE_CURRENT_SOURCE_DIR}/mgm_array.hpp
            ${CMAKE_CURRENT_SOURCE_DIR}/mgm_assert.hpp
            ${CMAKE_CURRENT_SOURCE_DIR}/mgm_function.hpp
            ${CMAKE_CURRENT_SOURCE_DIR}/mgm_hash.hpp
            ${CMAKE_CURRENT_SOURCE_DIR}/mgm_map.hpp
            ${CMAKE_CURRENT_SOURCE_DIR}/mgm_option.hpp
            ${CMAKE_CURRENT_SOURCE_DIR}/mgm_queues.hpp
            ${CMAKE_CURRENT_SOURCE_DIR}/mgm_set.hpp
            ${CMAKE_CURRENT_SOURCE_DIR}/mgm_span.hpp
            ${CMAKE_CURRENT_SOURCE_DIR}/mgm_string.hpp
            ${CMAKE_CURRENT_SOURCE_DIR}/mgm_threads.hpp
            ${CMAKE_CURRENT_SOURCE_DIR}/mgm_tuple.hpp
            ${CMAKE_CURRENT_SOURCE_DIR}/mgm_type_info.hpp
            ${CMAKE_CURRENT_SOURCE_DIR}/mgm_types.hpp
            ${CMAKE_CURRENT_SOURCE_DIR}/mgmath/mgmath.hpp

            ${CMAKE_CURRENT_SOURCE_DIR}/_padding.cpp
)

target_include_directories(
    MagmaStandardLibs
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/mgmath
)
set_target_properties(MagmaStandardLibs PROPERTIES LINKER_LANGUAGE CXX)
