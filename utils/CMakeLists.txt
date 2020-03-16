cmake_minimum_required(VERSION 3.7)
project (CHAR_ROM_GEN)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -pedantic -std=c++11")
set(CMAKE_CXX_FLAGS_DEBUG "-g -Ofast")
set(CMAKE_CXX_FLAGS_RELEASE "-g -Ofast")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(CHAR_ROM_GEN 2513SVG.cpp)
target_link_libraries(CHAR_ROM_GEN ${SDL2_LIBRARIES})
target_link_libraries(CHAR_ROM_GEN ${ImageMagick_LIBRARIES})
target_link_libraries(CHAR_ROM_GEN tbb)


#add_subdirectory(tests)



