# CMake Build Configuration for root of project
cmake_minimum_required(VERSION 3.18)
project(light_sampler VERSION 1.0 DESCRIPTION "Light dip detector" LANGUAGES C)

# Compiler options (inherited by sub-folders)
set(CMAKE_C_STANDARD 11)
add_compile_options(-Wall -Werror -Wpedantic -Wextra)
add_compile_options(-fdiagnostics-color)

# Address sanitizer: useful while developing, but it slows sampling and
# conflicts with valgrind. Disable with: cmake -S . -B build -DENABLE_ASAN=OFF
option(ENABLE_ASAN "Build with the address sanitizer" ON)
if(ENABLE_ASAN)
    add_compile_options(-fsanitize=address)
    add_link_options(-fsanitize=address)
endif()

# Enable PThread library for linking
add_compile_options(-pthread)
add_link_options(-pthread)

# What folders to build
add_subdirectory(hal)
add_subdirectory(app)
