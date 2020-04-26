project (wasm_client_tests)
cmake_minimum_required(VERSION 3.4)

set(CMAKE_CXX_COMPILER "em++")
add_definitions(--use-preload-plugins)
add_definitions(--preload-file resources)
add_definitions(-g -std=c++11 -O3 -s USE_SDL=2)
file(GLOB SOURCES *.cpp)
set(CMAKE_EXECUTABLE_SUFFIX ".wasm.js")
set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:im/lib/pkgconfig")

add_executable(app ${SOURCES})
include_directories(app templates)
include_directories(app src)

add_definitions( -DMAGICKCORE_QUANTUM_DEPTH=16 )
add_definitions( -DMAGICKCORE_HDRI_ENABLE=0 )
#find_package(ImageMagick COMPONENTS Magick++)
#include_directories(app ${ImageMagick_INCLUDE_DIRS})
#target_link_libraries(app ${ImageMagick_LIBRARIES})
include_directories(app im/include/ImageMagick-7)
link_directories(app im/lib/*.la)

#set_target_properties(app PROPERTIES LINK_FLAGS "-lm -s WASM=1 -s USE_ZLIB=1 -s USE_SDL=2 -s USE_FREETYPE=1 -s USE_LIBPNG=1 -s USE_LIBJPEG=1 --preload-file resources -s INITIAL_MEMORY=1GB -s ALLOW_MEMORY_GROWTH=1")
set_target_properties(app PROPERTIES LINK_FLAGS "-s DISABLE_EXCEPTION_CATCHING=2 -s LLD_REPORT_UNDEFINED -lm -s WASM=1 -s USE_SDL=2 --preload-file resources -s INITIAL_MEMORY=1GB -s ALLOW_MEMORY_GROWTH=1")


