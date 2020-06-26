cmake_minimum_required(VERSION 3.7)
project (SDL2_CRT_Decoder)
set(CMAKE_PROJECT_NAME SDL2_CRT_Decoder)
set(BUILD_TARGET FMAdecode.cpp)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra -pedantic -std=c++11")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O3")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

include_directories(${CMAKE_PROJECT_NAME} ../../templates)
include_directories(${CMAKE_PROJECT_NAME} ../../src)
include("../../dependencies.cmake")
