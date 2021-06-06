include_directories(${CMAKE_SOURCE_DIR}/Src)
link_directories(${CMAKE_BINARY_DIR}/Src)
set(CMAKE_CXX_STANDARD 20)

add_executable(dump1090 GPUPreparations.cpp main.cpp PacketHandling.cpp networking.cpp Expanded_set.cpp data_reader.cpp ../Headers/Modes.hpp Modes.cpp Utilities.cpp)