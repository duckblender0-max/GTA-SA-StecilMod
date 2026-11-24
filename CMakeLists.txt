cmake_minimum_required(VERSION 3.20)
project(StencilShadows LANGUAGES CXX)

add_library(StencilShadows SHARED
    src/StencilShadows.cpp
)

target_compile_features(StencilShadows PRIVATE cxx_std_17)
target_compile_definitions(StencilShadows PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(StencilShadows PRIVATE kernel32 user32)

set(PACKAGE_DIR "${CMAKE_BINARY_DIR}/package")
file(MAKE_DIRECTORY "${PACKAGE_DIR}")

set_target_properties(StencilShadows PROPERTIES
    OUTPUT_NAME "StencilShadows"
    PREFIX ""
    SUFFIX ".asi"
    RUNTIME_OUTPUT_DIRECTORY "${PACKAGE_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${PACKAGE_DIR}"
)

configure_file(config/StencilShadows.ini "${PACKAGE_DIR}/StencilShadows.ini" COPYONLY)
configure_file(shaders/StencilShadows.fx "${PACKAGE_DIR}/StencilShadows.fx" COPYONLY)
configure_file(package/StencilShadows.log "${PACKAGE_DIR}/StencilShadows.log" COPYONLY)
