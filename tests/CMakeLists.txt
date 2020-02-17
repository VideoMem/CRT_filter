cmake_minimum_required(VERSION 3.7)
project (SDL2_CRT_Tests)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra -pedantic")
set(CMAKE_CXX_FLAGS_DEBUG "-g -Ofast")
set(CMAKE_CXX_FLAGS_RELEASE "-Ofast")

find_package(SDL2 REQUIRED)
include_directories(SDL2_CRT_Tests ${SDL2_INCLUDE_DIRS})
include_directories(SDL2_CRT_Tests ../templates)
include_directories(SDL2_CRT_Tests ../include)
add_executable(SDL2_CRT_Tests All_tests.cpp)
target_link_libraries(SDL2_CRT_Tests ${SDL2_LIBRARIES})
