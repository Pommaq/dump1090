cmake_minimum_required(VERSION 3.19)
project(modes_binary)
include_directories(Src/)
include_directories(Headers/)

add_library(modes_binary
        Src/decoding.cpp Headers/decoding.h
        Src/modesMessage.cpp Headers/modesMessage.h
        )