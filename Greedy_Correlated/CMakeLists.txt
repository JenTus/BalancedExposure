cmake_minimum_required(VERSION 3.15)
project(Greedy_Correlated)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(Greedy_Correlated
        Greedy_Correlated.cpp
        rfw_timer.cpp
        rfw_timer.h)
