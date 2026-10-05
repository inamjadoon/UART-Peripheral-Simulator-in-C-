cmake_minimum_required(VERSION 3.16)
project(UARTSim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(UARTSim
    src/main.cpp
)

target_include_directories(UARTSim PRIVATE include)

if (MSVC)
    target_compile_options(UARTSim PRIVATE /W4)
else()
    target_compile_options(UARTSim PRIVATE -Wall -Wextra -Wpedantic)
endif()
