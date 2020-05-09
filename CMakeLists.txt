cmake_minimum_required(VERSION 3.7)
project (SDL_CRT_Filter)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -pedantic -std=c++11")
set(CMAKE_CXX_FLAGS_DEBUG "-g -Ofast")
set(CMAKE_CXX_FLAGS_RELEASE "-Ofast")

set(CMAKE_BUILD_TYPE Debug)
set(BUILD_TARGET CRTmain.cpp)

#includes
include_directories(${CMAKE_PROJECT_NAME} templates)
include_directories(${CMAKE_PROJECT_NAME} src)

include("dependencies.cmake")

add_subdirectory(tests)
add_subdirectory(utils)

