cmake_minimum_required(VERSION 3.31)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(vic)
add_compile_options(-Wall -Wextra -Wpedantic -Werror)

add_subdirectory(shaders)
add_subdirectory(sources)
