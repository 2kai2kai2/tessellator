cmake_minimum_required(VERSION 3.23)
project(tessellator)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(tessellator
        lib.h
        perlin.h
        svg.h
        tessellator.cpp)
