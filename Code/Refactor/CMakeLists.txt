cmake_minimum_required(VERSION 3.19)
set(CMAKE_CXX_STANDARD 20)
project(dump1090 CXX)

add_subdirectory("${CMAKE_SOURCE_DIR}/data sources")
add_subdirectory("${CMAKE_SOURCE_DIR}/Modes Handling")

add_executable(dump1090 main.cpp)
target_link_libraries(dump1090 data_sources modes_binary)