cmake_minimum_required(VERSION 3.12)
project(Tekst LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Automatically finds all your C++ source files
file(GLOB_RECURSE SOURCES "src/*.cpp") 

add_executable(tekst ${SOURCES})
