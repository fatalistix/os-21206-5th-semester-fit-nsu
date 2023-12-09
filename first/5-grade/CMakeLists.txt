cmake_minimum_required(VERSION 3.0)
project(main C)

set(CMAKE_C_STANDARD 11)

set(CMAKE_BINARY_DIR, "./bin")

set_target_properties(
  PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
             LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
             RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

add_executable(main main.c my-coroutine.h my-coroutine.c)
