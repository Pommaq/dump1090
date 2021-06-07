//
// Created by timmy on 2021-06-06.
//
#pragma once

#include <cstdint>
#include "aircraft.hpp"
#include "data_reader.hpp"
#include "Modes.hpp"
#include <cmath>



int decodeHexMessage(struct client *c);

uint32_t modesChecksum(const unsigned char *msg, int bits);

int modesMessageLenByType(int type);

int hexDigitVal(int c);

void computeMagnitudeVector();

int detectOutOfPhase(uint16_t const *m);

void applyPhaseCorrection(uint16_t *m);

void detectModeS(uint16_t *m, uint32_t mlen);

int decodeAC13Field(unsigned char *msg, int *unit);

int decodeAC12Field(unsigned char *msg, int *unit);


// ==== Error correction
int fixSingleBitErrors(unsigned char *msg, int bits);

int fixTwoBitsErrors(unsigned char *msg, int bits);

int bruteForceAP(unsigned char *msg, struct modesMessage *mm);

