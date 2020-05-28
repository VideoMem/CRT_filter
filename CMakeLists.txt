cmake_minimum_required(VERSION 3.7)
project (SDL_CRT_Filter C CXX)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -pedantic -std=c++11")
set(CMAKE_CXX_FLAGS_DEBUG "-g -Ofast")
set(CMAKE_CXX_FLAGS_RELEASE "-Ofast")

set(CMAKE_BUILD_TYPE Debug)
set(BUILD_TARGET CRTmain.cpp)

#includes
include_directories(${CMAKE_PROJECT_NAME} templates)
include_directories(${CMAKE_PROJECT_NAME} src)

include("dependencies.cmake")

#internals

add_library(CRT_FILTERS STATIC
        src/filters/BCS.hpp
        src/filters/Deflection.hpp
        src/filters/Filter.hpp
        src/filters/Noise.hpp
        src/filters/OSD.hpp
        src/filters/Sync.hpp
        src/generators/Generator.hpp
        src/generators/MagickOSD.hpp
        src/generators/Ripple.hpp
        src/loaders/fmt_tools/WaveFile.hpp
        src/loaders/LazySDL2.hpp
        src/loaders/MagickLoader.hpp
        src/loaders/ZMQLoader.hpp
        src/loaders/ZMQVideoPipe.hpp
        src/submodules/lpc/lpclient/lpc.hpp
        src/transcoders/libAVable.hpp
        src/transcoders/Magickable.hpp
        src/transcoders/Pixelable.hpp
        src/transcoders/Surfaceable.hpp
        src/transcoders/Transcodeable.hpp
        src/transcoders/Waveable.hpp
        src/picosha2.h
        src/ResourceRoller.hpp
        src/Config.hpp
        src/prngs.h
        src/BaseApp.hpp
        )

set_target_properties(CRT_FILTERS PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC ${CRT_FILTERS} )

add_subdirectory(tests)
add_subdirectory(utils)

