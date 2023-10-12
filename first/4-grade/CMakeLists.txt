cmake_minimum_required(VERSION 3.0)
project(main C)

set(CMAKE_C_STANDARD 11)

add_executable(main main.c my-core-threads.c my-core-threads.h)
