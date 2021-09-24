cmake_minimum_required(VERSION 3.19)
set(CMAKE_CXX_STANDARD 20)
project(data_sources DESCRIPTION "A generic interface to grab data from different data sources")
include_directories(Src/)
include_directories(Headers/)

message("Building library data_sources at ${PROJECT_SOURCE_DIR}")

add_library(
        data_sources STATIC
        Src/data_source.cpp Headers/data_source.h
        Src/radio.cpp Headers/radio.h Src/file.cpp Headers/file.h Src/rtlsdr.cpp Headers/rtlsdr.h)
