project(MagmaLibsInternal)

add_library(mgmath INTERFACE)
target_include_directories(mgmath INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/mgmath)
target_compile_definitions(mgmath INTERFACE MGMATH_SWIZZLE)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/mgmstd)
