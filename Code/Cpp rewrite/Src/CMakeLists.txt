include_directories(${CMAKE_SOURCE_DIR}/Src)
link_directories(${CMAKE_BINARY_DIR}/Src)
add_executable(dump1090 GPUPreparations.cpp main.cpp PacketHandling.cpp anet.c Expanded_set.cpp data_reader.cpp)