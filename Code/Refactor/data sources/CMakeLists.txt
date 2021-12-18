cmake_minimum_required(VERSION 3.19)
set(CMAKE_CXX_STANDARD 20)
project(data_sources DESCRIPTION "A generic interface to grab data from different data sources")

message("Building library data_sources at ${PROJECT_SOURCE_DIR}")

add_library(
        data_sources STATIC
        Src/DataSource.cpp Headers/DataSource.h
        Src/radio.cpp Headers/radio.h Src/file.cpp Headers/file.h Src/rtlsdr.cpp Headers/rtlsdr.h)

target_include_directories(data_sources PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/Src ${CMAKE_CURRENT_SOURCE_DIR}/Headers)
