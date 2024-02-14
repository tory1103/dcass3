cmake_minimum_required(VERSION 3.27)
project(dcass3)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Adds the executable
add_executable(
        dcass3
        source/dcass3/main.cpp
        source/dcass3/filesegmentation.cpp
)

include(FetchContent)
FetchContent_Declare(
    argparse
    GIT_REPOSITORY https://github.com/p-ranav/argparse.git
)
FetchContent_MakeAvailable(argparse)

target_link_libraries(dcass3 curl)
target_link_libraries(dcass3 argparse)