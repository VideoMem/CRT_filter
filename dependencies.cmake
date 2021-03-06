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

pkg_check_modules(VIPS REQUIRED vips-cpp)

pkg_check_modules(TURBOFEC REQUIRED turbofec)

pkg_check_modules(ZLIB REQUIRED zlib)

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

# To find and use ffmpeg avcodec library
find_library( AVCODEC_LIBRARY avcodec )
find_library( AVUTIL_LIBRARY avutil )
find_library( AVFORMAT_LIBRARY avformat )

#pthread
find_package (Threads)

#find_package (TurboFEC)
find_package (ZLIB)

##executable / link section
add_executable(${CMAKE_PROJECT_NAME} ${BUILD_TARGET})
target_link_libraries(${CMAKE_PROJECT_NAME} ${SDL2_LIBRARIES})
target_link_libraries(${CMAKE_PROJECT_NAME} ${ImageMagick_LIBRARIES})

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC ${ZeroMQ_INCLUDE_DIR})
target_link_libraries(${CMAKE_PROJECT_NAME} ${ZeroMQ_LIBRARY})

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC ${GRRUNTIME_INCLUDE_DIRS})
target_link_libraries(${CMAKE_PROJECT_NAME} ${GRRUNTIME_LIBRARY})

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC ${VIPS_INCLUDE_DIRS})
target_link_libraries(${CMAKE_PROJECT_NAME} ${VIPS_LIBRARIES})

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC ${TURBOFEC_INCLUDE_DIRS})
target_link_libraries(${CMAKE_PROJECT_NAME} ${TURBOFEC_LIBRARIES})

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC ${ZLIB_INCLUDE_DIRS})
target_link_libraries(${CMAKE_PROJECT_NAME} ${ZLIB_LIBRARIES})


target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC ${AVCODEC_INCLUDE_DIRS})
target_link_libraries(${CMAKE_PROJECT_NAME} ${AVCODEC_LIBRARY})
target_link_libraries(${CMAKE_PROJECT_NAME} ${AVUTIL_LIBRARY})
target_link_libraries(${CMAKE_PROJECT_NAME} ${AVFORMAT_LIBRARY})

target_link_libraries(${CMAKE_PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})

