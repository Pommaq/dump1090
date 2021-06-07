//
// Created by timmy on 2021-06-06.
//
#pragma once

#include <cstdint>
#include "aircraft.hpp"
#include "data_reader.hpp"
#include "Modes.hpp"
#include <cmath>

/* Parity table for MODE S Messages.
 * The table contains 112 elements, every element corresponds to a bit set
 * in the bitf_message, starting from the first bit of actual data after the
 * preamble.
 *
 * For messages of 112 bit, the whole table is used.
 * For messages of 56 bits only the last 56 elements are used.
 *
 * The algorithm is as simple as xoring all the elements in this table
 * for which the corresponding bit on the bitf_message is set to 1.
 *
 * The latest 24 elements in this table are set to 0 as the checksum at the
 * end of the bitf_message should not affect the computation.
 *
 * Note: this function can be used with DF11 and DF17, other modes have
 * the CRC xored with the sender address as they are reply to interrogations,
 * but a casual listener can't split the address from the checksum.
 */

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

