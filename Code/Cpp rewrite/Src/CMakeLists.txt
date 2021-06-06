include_directories(${CMAKE_SOURCE_DIR}/Src)
include_directories(${CMAKE_SOURCE_DIR}/Headers)
link_directories(${CMAKE_BINARY_DIR}/Src)
link_directories(${CMAKE_BINARY_DIR}/Headers)

set(CMAKE_CXX_STANDARD 20)

add_executable(dump1090 ../Headers/GPUPreparations.hpp GPUPreparations.cpp ../Headers/main.hpp main.cpp
        ../Headers/PacketHandling.hpp PacketHandling.cpp
        ../Headers/anet.hpp anet.cpp
        ../Headers/Expanded_set.hpp Expanded_set.cpp
        ../Headers/data_reader.hpp data_reader.cpp
        ../Headers/Modes.hpp Modes.cpp
        ../Headers/Utilities.hpp Utilities.cpp
        ../Headers/Networking.hpp Networking.cpp Terminal.cpp ../Headers/Terminal.h aircraft.cpp ../Headers/aircraft.h Decoding.cpp ../Headers/Decoding.h debugging.cpp ../Headers/debugging.h)