cmake_minimum_required(VERSION 3.16)

project(part1)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_executable(part1
    src/main.c
    src/crc8/crc8.c
    src/payload/payload.c
    src/test/test.c
)

target_include_directories(part1 PRIVATE
    src/crc8/include
    src/payload/include
    src/test/include
)

target_compile_options(part1 PRIVATE
    -Wall
    -Wextra
)
