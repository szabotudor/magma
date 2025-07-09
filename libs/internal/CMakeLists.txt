project(MagmaLibsInternal)

add_library(mgmath INTERFACE)
target_compile_definitions(mgmath INTERFACE MGMATH_SWIZZLE)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/mgmstd)
