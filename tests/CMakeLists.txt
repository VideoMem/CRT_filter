cmake_minimum_required(VERSION 3.7)
project (SDL2_CRT_Tests)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra -pedantic -std=c++11")
set(CMAKE_CXX_FLAGS_DEBUG "-g -Ofast")
set(CMAKE_CXX_FLAGS_RELEASE "-Ofast")

find_package(SDL2 REQUIRED)
include_directories(SDL2_CRT_Tests ${SDL2_INCLUDE_DIRS})
include_directories(SDL2_CRT_Tests ../templates)
include_directories(SDL2_CRT_Tests ../src)
add_executable(SDL2_CRT_Tests All_tests.cpp filesystem_Tests.h)
target_link_libraries(SDL2_CRT_Tests ${SDL2_LIBRARIES})

add_definitions( -DMAGICKCORE_QUANTUM_DEPTH=16 )
add_definitions( -DMAGICKCORE_HDRI_ENABLE=0 )
find_package(ImageMagick COMPONENTS Magick++)
find_package(ImageMagick COMPONENTS MagickCore)
include_directories(${ImageMagick_INCLUDE_DIRS})
target_link_libraries(SDL2_CRT_Tests ${ImageMagick_LIBRARIES})