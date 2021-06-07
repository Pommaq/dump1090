//
// Created by timmy on 2021-06-06.
//
#pragma once

#include <cstdint>
#include "aircraft.hpp"
#include "data_reader.hpp"
#include "Modes.hpp"
#include <cmath>

/* The struct we use to store information about a decoded bitf_message. */
struct modesMessage {
    modesMessage() = default;
    ~modesMessage() = default;
    modesMessage(unsigned char *msg);
    void updatePlanes();
        /* Generic fields */
    unsigned char msg[MODES_LONG_MSG_BYTES]; /* Binary bitf_message. */
    int msgbits;                /* Number of bits in bitf_message */
    int msgtype;                /* Downlink format # */
    int crcok;                  /* True if CRC was valid */
    uint32_t crc;               /* Message CRC */
    int errorbit;               /* Bit corrected. -1 if no bit corrected. */
    int aa1, aa2, aa3;          /* ICAO Address bytes 1 2 and 3 */
    int phase_corrected;        /* True if phase correction was applied. */

    /* DF 11 */
    int ca;                     /* Responder capabilities. */

    /* DF 17 */
    int metype;                 /* Extended squitter bitf_message type. */
    int mesub;                  /* Extended squitter bitf_message subtype. */
    int heading_is_valid;
    int heading;
    int aircraft_type;
    int fflag;                  /* 1 = Odd, 0 = Even CPR bitf_message. */
    int tflag;                  /* UTC synchronized? */
    int raw_latitude;           /* Non decoded latitude */
    int raw_longitude;          /* Non decoded longitude */
    char flight[9];             /* 8 chars flight number. */
    int ew_dir;                 /* 0 = East, 1 = West. */
    int ew_velocity;            /* E/W velocity. */
    int ns_dir;                 /* 0 = North, 1 = South. */
    int ns_velocity;            /* N/S velocity. */
    int vert_rate_source;       /* Vertical rate source. */
    int vert_rate_sign;         /* Vertical rate sign. */
    int vert_rate;              /* Vertical rate. */
    int velocity;               /* Computed from EW and NS velocity. */

    /* DF4, DF5, DF20, DF21 */
    int fs;                     /* Flight status for DF4,5,20,21 */
    int dr;                     /* Request extraction of downlink request. */
    int um;                     /* Request extraction of downlink request. */
    int identity;               /* 13 bits identity (Squawk). */

    /* Fields used by multiple bitf_message types. */
    int altitude, unit;
};


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

uint32_t modesChecksum(unsigned char *msg, int bits);

int modesMessageLenByType(int type);

int hexDigitVal(int c);

int cprModFunction(int a, int b);

int cprNLFunction(double lat);

int cprNFunction(double lat, int isodd);

double cprDlonFunction(double lat, int isodd);

void decodeCPR(aircraft *a);

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

uint32_t ICAOCacheHashAddress(uint32_t a);

void addRecentlySeenICAOAddr(uint32_t addr);

int ICAOAddressWasRecentlySeen(uint32_t addr);

