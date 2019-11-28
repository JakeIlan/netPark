cmake_minimum_required(VERSION 3.15)
project(net C)

set(CMAKE_C_STANDARD 99)

add_executable(net client.c server.c)