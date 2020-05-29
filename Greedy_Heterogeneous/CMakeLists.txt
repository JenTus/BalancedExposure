cmake_minimum_required(VERSION 3.15)
project(Greedy_Heterogeneous)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(Greedy_Heterogeneous
        Greedy_Heterogeneous.cpp
        rfw_timer.cpp
        rfw_timer.h)
