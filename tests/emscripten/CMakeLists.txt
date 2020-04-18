project (wasm_client)
cmake_minimum_required(VERSION 3.4)

set(CMAKE_CXX_COMPILER "em++")

add_definitions(-g -O3)

file(GLOB SOURCES *.cpp)
set(CMAKE_EXECUTABLE_SUFFIX ".wasm.js")

add_executable(app ${SOURCES})
set_target_properties(app PROPERTIES LINK_FLAGS "-lm -s WASM=1 -s USE_SDL=2")


