cmake_minimum_required(VERSION 3.7)
project (SDL_CRT_Filter)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra -pedantic -std=c++11")
set(CMAKE_CXX_FLAGS_DEBUG "-g -Ofast")
set(CMAKE_CXX_FLAGS_RELEASE "-Ofast")

find_package(SDL2 REQUIRED)
#find_package(ImageMagick REQUIRED)
include_directories(SDL_CRT_Filter ${SDL2_INCLUDE_DIRS})
#include_directories(SDL_CRT_Filter ${ImageMagick_INCLUDE_DIRS})

add_executable(SDL_CRT_Filter CRTmain.cpp)

#target_link_libraries(SDL_CRT_Filter ${ImageMagick_LIBRARIES})
target_link_libraries(SDL_CRT_Filter ${SDL2_LIBRARIES})

add_subdirectory(tests)

add_definitions( -DMAGICKCORE_QUANTUM_DEPTH=32 )
add_definitions( -DMAGICKCORE_HDRI_ENABLE=0 )
find_package(ImageMagick COMPONENTS Magick++)
include_directories(${ImageMagick_INCLUDE_DIRS})
target_link_libraries(SDL_CRT_Filter ${ImageMagick_LIBRARIES})