##SDL2 section
find_package(SDL2 REQUIRED)
include_directories(${CMAKE_PROJECT_NAME} ${SDL2_INCLUDE_DIRS})

##Magick++ Section
add_definitions( -DMAGICKCORE_QUANTUM_DEPTH=16 )
add_definitions( -DMAGICKCORE_HDRI_ENABLE=0 )
find_package(ImageMagick COMPONENTS Magick++)
include_directories(${ImageMagick_INCLUDE_DIRS})

##ZMQ Section

## load in pkg-config support
find_package(PkgConfig)
## use pkg-config to get hints for 0mq locations
pkg_check_modules(PC_ZeroMQ REQUIRED libzmq)

## use the hint from above to find where 'zmq.hpp' is located
#find_path(ZeroMQ_INCLUDE_DIR
#        NAMES zmq.hpp
#        PATHS ${PC_ZeroMQ_INCLUDE_DIRS}
#        )

## use the hint from about to find the location of libzmq
#find_library(ZeroMQ_LIBRARY
#        NAMES zmq
#        PATHS ${PC_ZeroMQ_LIBRARY_DIRS}
#        )

#Gnuradio Section
set(ENV{PKG_CONFIG_PATH}  "~/sdr/lib/pkgconfig")
pkg_check_modules(GRRUNTIME REQUIRED gnuradio-runtime )

#find_path(GRRUNTIME_INCLUDE_DIR
#        NAMES pmt.h
#        PATHS ${PC_GRRUNTIME_INCLUDE_DIRS}
#        )

#find_path(GrRuntime_LIBRARY
#        NAMES gnuradio-pmt
#        PATHS ${PC_GrRuntime_LIBRARY_DIRS}
#        )

##executable / link section
add_executable(${CMAKE_PROJECT_NAME} ${BUILD_TARGET})
target_link_libraries(${CMAKE_PROJECT_NAME} ${SDL2_LIBRARIES})
target_link_libraries(${CMAKE_PROJECT_NAME} ${ImageMagick_LIBRARIES})

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC ${ZeroMQ_INCLUDE_DIR})
target_link_libraries(${CMAKE_PROJECT_NAME} ${ZeroMQ_LIBRARY})

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC ${GRRUNTIME_INCLUDE_DIRS})
target_link_libraries(${CMAKE_PROJECT_NAME} ${GRRUNTIME_LIBRARY})
