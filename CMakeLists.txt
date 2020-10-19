cmake_minimum_required(VERSION 3.17)
project(gpgpu_hw1 C)

set(CMAKE_C_STANDARD 99)

set(CMAKE_BUILD_TYPE Debug)

add_executable(main main.c)
target_link_libraries(main pthread)

add_executable(main2 main2.c)

add_executable(main3 main3.c)

add_executable(main4 main4.c)
target_link_libraries(main4 pthread)