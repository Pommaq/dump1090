set(CMAKE_CXX_STANDARD 20)
include_directories(${CMAKE_SOURCE_DIR}/Src)
include_directories(${CMAKE_SOURCE_DIR}/Headers)
link_directories(${CMAKE_BINARY_DIR}/Src)
link_directories(${CMAKE_BINARY_DIR}/Headers)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
set(CMAKE_HAVE_THREADS_LIBRARY 1)
set(CMAKE_USE_WIN32_THREADS_INIT 0)
set(CMAKE_USE_PTHREADS_INIT 1)
set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(rtlsdr REQUIRED)


set(CMAKE_CXX_STANDARD 20)

add_executable(dump1090
        #Src/GPUPreparations.cpp Headers/GPUPreparations.hpp
        Src/main.cpp
        Src/anet.cpp Headers/anet.hpp
        Src/PacketHandling.cpp Headers/PacketHandling.hpp
        #Src/Expanded_set.cpp Headers/Expanded_set.hpp
        Src/data_reader.cpp Headers/data_reader.hpp
        Src/Modes.cpp Headers/Modes.hpp
        Src/Utilities.cpp Headers/Utilities.hpp
        Src/Networking.cpp Headers/Networking.hpp
        Src/Terminal.cpp Headers/Terminal.hpp
        Src/aircraft.cpp Headers/aircraft.hpp
        Src/Decoding.cpp Headers/Decoding.hpp
        Src/debugging.cpp Headers/debugging.hpp Src/modesMessage.cpp Headers/modesMessage.h)

target_link_libraries(dump1090 rtlsdr )#OpenCL::OpenCL)