//
// Created by timmy on 2021-05-11.
//

#ifndef DUMP1090_MAIN_HPP
#define DUMP1090_MAIN_HPP
#include <cstring>
#include <csignal>
#include <fstream>
#include <iostream>

#include "../Headers/aircraft.h"
#include "../Headers/anet.hpp"
#include "../Headers/data_reader.hpp"
#include "../Headers/debugging.h"
#include "../Headers/Decoding.h"
#include "../Headers/Expanded_set.hpp"
#include "../Headers/GPUPreparations.hpp"
#include "../Headers/Modes.hpp"
#include "../Headers/Networking.hpp"
#include "../Headers/PacketHandling.hpp"
#include "../Headers/Terminal.h"
#include "../Headers/Utilities.hpp"


void snipMode(int level);
void backgroundTasks();
int main(int argc, char **argv);
#endif //DUMP1090_MAIN_HPP
