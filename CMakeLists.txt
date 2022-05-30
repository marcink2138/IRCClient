cmake_minimum_required(VERSION 3.22)
project(IRCClient C)

set(CMAKE_C_STANDARD 90)
set(CMAKE_C_FLAGS -pthread)
add_executable(IRCClient main.c)
