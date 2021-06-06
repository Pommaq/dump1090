//
// Created by timmy on 2021-06-06.
//

#ifndef DUMP1090_TERMINAL_H
#define DUMP1090_TERMINAL_H

#include "../Headers/Decoding.h"

int getTermRows();

void interactiveShowData();

void sigWinchCallback();

void displayModesMessage(modesMessage *mm);
char *getMEDescription(int metype, int mesub);
/* Capability table. */
char *ca_str[8] = {
        /* 0 */ "Level 1 (Survillance Only)",
        /* 1 */ "Level 2 (DF0,4,5,11)",
        /* 2 */ "Level 3 (DF0,4,5,11,20,21)",
        /* 3 */ "Level 4 (DF0,4,5,11,20,21,24)",
        /* 4 */ "Level 2+3+4 (DF0,4,5,11,20,21,24,code7 - is on ground)",
        /* 5 */ "Level 2+3+4 (DF0,4,5,11,20,21,24,code7 - is on airborne)",
        /* 6 */ "Level 2+3+4 (DF0,4,5,11,20,21,24,code7)",
        /* 7 */ "Level 7 ???"
};

/* Flight status table. */
char *fs_str[8] = {
        /* 0 */ "Normal, Airborne",
        /* 1 */ "Normal, On the ground",
        /* 2 */ "ALERT,  Airborne",
        /* 3 */ "ALERT,  On the ground",
        /* 4 */ "ALERT & Special Position Identification. Airborne or Ground",
        /* 5 */ "Special Position Identification. Airborne or Ground",
        /* 6 */ "Value 6 is not assigned",
        /* 7 */ "Value 7 is not assigned"
};

#endif //DUMP1090_TERMINAL_H
