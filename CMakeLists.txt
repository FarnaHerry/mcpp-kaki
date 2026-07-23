cmake_minimum_required(VERSION 3.17)
project(cpp-kaki VERSION 1.0 DESCRIPTION "A xianxia Metroidvania action game" LANGUAGES CXX)

# Must be set before add_subdirectory(godot-cpp)
set(GODOTCPP_TARGET template_debug CACHE STRING "Build target: template_debug, template_release, editor")
set(GODOTCPP_API_VERSION "4.6" CACHE STRING "Godot API version")

add_subdirectory(godot-cpp)

# Extension library
set(LIBNAME "cpp-kaki")

# Get auto-generated suffix from godot-cpp (e.g. .linux.template_debug.x86_64)
get_target_property(GODOTCPP_SUFFIX godot-cpp GODOTCPP_SUFFIX)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    src/*.cpp
    src/*.h
)

add_library(${LIBNAME} SHARED
    ${SOURCES}
)

target_link_libraries(${LIBNAME} PRIVATE godot::cpp)
# Our extension code uses C++23 (GDExtension is C ABI; godot-cpp itself stays at its own C++17).
target_compile_features(${LIBNAME} PRIVATE cxx_std_23)

# Output: bin/libcpp-kaki.linux.template_debug.x86_64.so
set_target_properties(${LIBNAME} PROPERTIES
    OUTPUT_NAME "${LIBNAME}${GODOTCPP_SUFFIX}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)
