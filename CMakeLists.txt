cmake_minimum_required(VERSION 3.10)
project(LogSystem)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

add_executable(LogSystem
    src/LogManager.h
    src/LogManager.cpp
    src/LogStream.h
    src/LogStream.cpp
    src/main.cpp
)

