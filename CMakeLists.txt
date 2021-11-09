cmake_minimum_required(VERSION 3.20)
project(sockprog)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(sockprog
        src/client.cpp src/server.cpp)
