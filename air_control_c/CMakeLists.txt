cmake_minimum_required(VERSION 3.10)
project(air_control_c C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Header directory
include_directories(${CMAKE_SOURCE_DIR}/include)

# Executable target
add_executable(${CMAKE_PROJECT_NAME}
    src/main.c
    src/functions.c
)