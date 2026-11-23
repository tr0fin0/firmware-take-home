cmake_minimum_required(VERSION 3.16)

project(part2)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_executable(part2
    src/flash/flash.c
    src/main.c
    src/test/test.c
)

target_include_directories(part2 PRIVATE
    src/flash/include
    src/queue/include
    src/registry/include
    src/test/include
)

target_compile_options(part2 PRIVATE
    -Wall
    -Wextra
)
