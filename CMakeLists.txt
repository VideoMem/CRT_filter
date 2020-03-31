cmake_minimum_required(VERSION 3.7)
project (SDL_CRT_Filter)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -pedantic -std=c++11")
set(CMAKE_CXX_FLAGS_DEBUG "-g ")
set(CMAKE_CXX_FLAGS_RELEASE "-g -Ofast")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

##SDL2 section
find_package(SDL2 REQUIRED)
include_directories(SDL_CRT_Filter ${SDL2_INCLUDE_DIRS})
include_directories(SDL_CRT_Filter templates)
include_directories(SDL_CRT_Filter src)

##Magick++ Section
add_definitions( -DMAGICKCORE_QUANTUM_DEPTH=16 )
add_definitions( -DMAGICKCORE_HDRI_ENABLE=0 )
find_package(ImageMagick COMPONENTS Magick++)
include_directories(${ImageMagick_INCLUDE_DIRS})

add_executable(SDL_CRT_Filter CRTmain.cpp)
target_link_libraries(SDL_CRT_Filter ${SDL2_LIBRARIES})
target_link_libraries(SDL_CRT_Filter ${ImageMagick_LIBRARIES})
#target_link_libraries(SDL_CRT_Filter tbb)


add_subdirectory(tests)
add_subdirectory(utils)


