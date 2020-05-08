cmake_minimum_required(VERSION 3.7)
project (SDL_CRT_Filter)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -pedantic -std=c++11")
set(CMAKE_CXX_FLAGS_DEBUG "-g -Ofast")
set(CMAKE_CXX_FLAGS_RELEASE "-Ofast")

set(CMAKE_BUILD_TYPE Debug)

#includes
include_directories(CMAKE_PROJECT_NAME templates)
include_directories(CMAKE_PROJECT_NAME src)

##SDL2 section
find_package(SDL2 REQUIRED)
include_directories(CMAKE_PROJECT_NAME ${SDL2_INCLUDE_DIRS})

##Magick++ Section
add_definitions( -DMAGICKCORE_QUANTUM_DEPTH=16 )
add_definitions( -DMAGICKCORE_HDRI_ENABLE=0 )
find_package(ImageMagick COMPONENTS Magick++)
include_directories(${ImageMagick_INCLUDE_DIRS})

##ZMQ Section

## load in pkg-config support
find_package(PkgConfig)
## use pkg-config to get hints for 0mq locations
pkg_check_modules(PC_ZeroMQ QUIET zmq)

## use the hint from above to find where 'zmq.hpp' is located
find_path(ZeroMQ_INCLUDE_DIR
        NAMES zmq.hpp
        PATHS ${PC_ZeroMQ_INCLUDE_DIRS}
        )

## use the hint from about to find the location of libzmq
find_library(ZeroMQ_LIBRARY
        NAMES zmq
        PATHS ${PC_ZeroMQ_LIBRARY_DIRS}
        )


##executable / link section
add_executable(CMAKE_PROJECT_NAME CRTmain.cpp)
target_link_libraries(CMAKE_PROJECT_NAME ${SDL2_LIBRARIES})
target_link_libraries(CMAKE_PROJECT_NAME ${ImageMagick_LIBRARIES})
#target_link_libraries(CMAKE_PROJECT_NAME tbb)
target_include_directories(CMAKE_PROJECT_NAME PUBLIC ${ZeroMQ_INCLUDE_DIR})
target_link_libraries(CMAKE_PROJECT_NAME ${ZeroMQ_LIBRARY})

add_subdirectory(tests)
add_subdirectory(utils)

