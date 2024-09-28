cmake_minimum_required(VERSION 3.27)
project(program1 C)

set(CMAKE_C_STANDARD 11)

add_library(program1 STATIC library.c
        h1-counter.c
        h1-counter.c)
