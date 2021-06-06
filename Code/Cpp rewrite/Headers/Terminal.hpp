//
// Created by timmy on 2021-06-06.
//

#ifndef DUMP1090_TERMINAL_HPP
#define DUMP1090_TERMINAL_HPP

#include "../Headers/Decoding.hpp"

int getTermRows();

void interactiveShowData();

void sigWinchCallback();

void displayModesMessage(modesMessage *mm);
std::string & getMEDescription(int metype, int mesub);
/* Capability table. */


#endif //DUMP1090_TERMINAL_HPP
