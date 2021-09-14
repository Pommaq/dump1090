cmake_minimum_required(VERSION 3.19)
set(CMAKE_CXX_STANDARD 20)
project(coolbeans CXX)
include_directories(${PROJECT_SOURCE_DIR}/Src)
include_directories(${PROJECT_SOURCE_DIR}/Headers)
link_directories(${CMAKE_BINARY_DIR}/Src)
link_directories(${CMAKE_BINARY_DIR}/Headers)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
set(CMAKE_HAVE_THREADS_LIBRARY 1)
set(CMAKE_USE_WIN32_THREADS_INIT 0)
set(CMAKE_USE_PTHREADS_INIT 1)
set(THREADS_PREFER_PTHREAD_FLAG ON)


find_library(RTLSDR rtlsdr REQUIRED )

set(CMAKE_CXX_STANDARD 20)

add_executable(coolbeans
        Src/main.cpp
        Src/modesMessage.cpp Headers/modesMessage.h Src/main.cpp Src/decoding.cpp Src/decoding.h)

target_link_libraries(coolbeans ${RTLSDR} )