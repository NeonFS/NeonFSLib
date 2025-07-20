cmake_minimum_required(VERSION 3.31)
project(NeonFSLib)

set(CMAKE_CXX_STANDARD 17)

add_library(NeonFSLib STATIC NeonFSLib.cpp
        include/NeonFS/result.hpp)
