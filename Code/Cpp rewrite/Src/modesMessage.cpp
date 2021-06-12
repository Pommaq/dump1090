//
// Created by timmy on 2021-06-07.
//

#include <cstring>
#include "../Headers/modesMessage.h"
#include "../Headers/Decoding.hpp"
#include "../Headers/Networking.hpp"
#include "../Headers/Terminal.hpp"
#include "../Headers/Utilities.hpp"

/* Decode a raw Mode S bitf_message demodulated as a stream of bytes by
 * detectModeS(), and split it into fields populating a modesMessage
 * structure. */
modesMessage::modesMessage(unsigned char *msg) {

    uint32_t crc2;   /* Computed CRC, used to verify the bitf_message CRC. */
    const char *ais_charset = "?ABCDEFGHIJKLMNOPQRSTUVWXYZ????? ???????????????0123456789??????";

    /* Work on our local copy */
    memcpy(this->msg, msg, MODES_LONG_MSG_BYTES);
    msg = this->msg;

    /* Get the bitf_message type ASAP as other operations depend on this */
    this->msgtype = msg[0] >> 3;    /* Downlink Format */
    this->msgbits = modesMessageLenByType(this->msgtype);

    /* CRC is always the last three bytes. */
    this->crc = ((uint32_t) msg[(this->msgbits / 8) - 3] << 16) |
                ((uint32_t) msg[(this->msgbits / 8) - 2] << 8) |
                (uint32_t) msg[(this->msgbits / 8) - 1];
    crc2 = modesChecksum(msg, this->msgbits);

    /* Check CRC and fix single bit errors using the CRC when
     * possible (DF 11 and 17). */
    this->errorbit = -1;  /* No error */
    this->crcok = (this->crc == crc2);

    if (!this->crcok && Modes.fix_errors &&
        (this->msgtype == 11 || this->msgtype == 17)) {
        // We need to fix it or drop this packet
        if (Modes.aggressive && this->msgtype == 17) {
            if ((this->errorbit = fixTwoBitsErrors(msg, this->msgbits)) != -1) {
                this->crc = modesChecksum(msg, this->msgbits);
                this->crcok = 1;
            }
        }
        else if ((this->errorbit = fixSingleBitErrors(msg, this->msgbits)) != -1) {
            if (this->errorbit == -2) {
                exit(EXIT_FAILURE);
            }
            this->crc = modesChecksum(msg, this->msgbits);
            this->crcok = 1;
        }

    }

    /* Note that most of the other computation happens *after* we fix
     * the single bit errors, otherwise we would need to recompute the
     * fields again. */
    this->ca = msg[0] & 7;        /* Responder capabilities. */

    /* ICAO address */
    this->aa1 = msg[1];
    this->aa2 = msg[2];
    this->aa3 = msg[3];

    /* DF 17 type (assuming this is a DF17, otherwise not used) */
    this->metype = msg[4] >> 3;   /* Extended squitter bitf_message type. */
    this->mesub = msg[4] & 7;     /* Extended squitter bitf_message subtype. */

    /* Fields for DF4,5,20,21 */
    this->fs = msg[0] & 7;        /* Flight status for DF4,5,20,21 */
    this->dr = msg[1] >> 3 & 31;  /* Request extraction of downlink request. */
    this->um = ((msg[1] & 7) << 3) | /* Request extraction of downlink request. */
               msg[2] >> 5;

    /* In the squawk (identity) field bits are interleaved like that
     * (bitf_message bit 20 to bit 32):
     *
     * C1-A1-C2-A2-C4-A4-ZERO-B1-D1-B2-D2-B4-D4
     *
     * So every group of three bits A, B, C, D represent an integer
     * from 0 to 7.
     *
     * The actual meaning is just 4 octal numbers, but we convert it
     * into a base ten number tha happens to represent the four
     * octal numbers.
     *
     * For more info: http://en.wikipedia.org/wiki/Gillham_code */
    {
        int a, b, c, d;

        a = ((msg[3] & 0x80) >> 5) |
            ((msg[2] & 0x02) >> 0) |
            ((msg[2] & 0x08) >> 3);
        b = ((msg[3] & 0x02) << 1) |
            ((msg[3] & 0x08) >> 2) |
            ((msg[3] & 0x20) >> 5);
        c = ((msg[2] & 0x01) << 2) |
            ((msg[2] & 0x04) >> 1) |
            ((msg[2] & 0x10) >> 4);
        d = ((msg[3] & 0x01) << 2) |
            ((msg[3] & 0x04) >> 1) |
            ((msg[3] & 0x10) >> 4);
        this->identity = a * 1000 + b * 100 + c * 10 + d;
    }

    /* DF 11 & 17: try to populate our ICAO addresses whitelist.
     * DFs with an AP field (xored addr and crc), try to decode it. */
    if (this->msgtype != 11 && this->msgtype != 17) {
        /* Check if we can check the checksum for the Downlink Formats where
         * the checksum is xored with the aircraft ICAO address. We try to
         * brute force it using a list of recently seen aircraft addresses. */
        if (this->bruteForceAP(msg)) {
            /* We recovered the bitf_message, mark the checksum as valid. */
            this->crcok = 1;
        } else {
            this->crcok = 0;
        }
    } else {
        /* If this is DF 11 or DF 17 and the checksum was ok,
         * we can add this address to the list of recently seen
         * addresses. */
        if (this->crcok && this->errorbit == -1) {
            uint32_t addr = (this->aa1 << 16) | (this->aa2 << 8) | this->aa3;
            addRecentlySeenICAOAddr(addr);
        }
    }

    /* Decode 13 bit altitude for DF0, DF4, DF16, DF20 */
    if (this->msgtype == 0 || this->msgtype == 4 ||
        this->msgtype == 16 || this->msgtype == 20) {
        this->altitude = decodeAC13Field(msg, &this->unit);
    }

    /* Decode extended squitter specific stuff. */
    if (this->msgtype == 17) {
        /* Decode the extended squitter bitf_message. */

        if (this->metype >= 1 && this->metype <= 4) {
            /* Aircraft Identification and Category */
            this->aircraft_type = this->metype - 1;
            this->flight[0] = ais_charset[msg[5] >> 2];
            this->flight[1] = ais_charset[((msg[5] & 3) << 4) | (msg[6] >> 4)];
            this->flight[2] = ais_charset[((msg[6] & 15) << 2) | (msg[7] >> 6)];
            this->flight[3] = ais_charset[msg[7] & 63];
            this->flight[4] = ais_charset[msg[8] >> 2];
            this->flight[5] = ais_charset[((msg[8] & 3) << 4) | (msg[9] >> 4)];
            this->flight[6] = ais_charset[((msg[9] & 15) << 2) | (msg[10] >> 6)];
            this->flight[7] = ais_charset[msg[10] & 63];
            this->flight[8] = '\0';
        } else if (this->metype >= 9 && this->metype <= 18) {
            /* Airborne position Message */
            this->fflag = msg[6] & (1 << 2);
            this->tflag = msg[6] & (1 << 3);
            this->altitude = decodeAC12Field(msg, &this->unit);
            this->raw_latitude = ((msg[6] & 3) << 15) |
                                 (msg[7] << 7) |
                                 (msg[8] >> 1);
            this->raw_longitude = ((msg[8] & 1) << 16) |
                                  (msg[9] << 8) |
                                  msg[10];
        } else if (this->metype == 19 && this->mesub >= 1 && this->mesub <= 4) {
            /* Airborne Velocity Message */
            if (this->mesub == 1 || this->mesub == 2) {
                this->ew_dir = (msg[5] & 4) >> 2;
                this->ew_velocity = ((msg[5] & 3) << 8) | msg[6];
                this->ns_dir = (msg[7] & 0x80) >> 7;
                this->ns_velocity = ((msg[7] & 0x7f) << 3) | ((msg[8] & 0xe0) >> 5);
                this->vert_rate_source = (msg[8] & 0x10) >> 4;
                this->vert_rate_sign = (msg[8] & 0x8) >> 3;
                this->vert_rate = ((msg[8] & 7) << 6) | ((msg[9] & 0xfc) >> 2);
                /* Compute velocity and angle from the two speed
                 * components. */
                this->velocity = sqrt(this->ns_velocity * this->ns_velocity +
                                      this->ew_velocity * this->ew_velocity);
                if (this->velocity) {
                    int ewv = this->ew_velocity;
                    int nsv = this->ns_velocity;
                    double direction;

                    if (this->ew_dir) ewv *= -1;
                    if (this->ns_dir) nsv *= -1;
                    direction = atan2(ewv, nsv);

                    /* Convert to degrees. */
                    this->heading = direction * 360 / (M_PI * 2);
                    /* We don't want negative values but a 0-360 scale. */
                    if (this->heading < 0) this->heading += 360;
                } else {
                    this->heading = 0;
                }
            } else if (this->mesub == 3 || this->mesub == 4) {
                this->heading_is_valid = msg[5] & (1 << 2);
                this->heading = (360.0 / 128) * (((msg[5] & 3) << 5) |
                                                 (msg[6] >> 3));
            }
        }
    }
    this->phase_corrected = 0; /* Set to 1 by the caller if needed. */
}

/* When a new bitf_message is available, because it was decoded from the
 * RTL device, file, or received in the TCP input port, or any other
 * way we can receive a decoded bitf_message, we call this function in order
 * to use the bitf_message.
 *
 * Basically this function passes a raw bitf_message to the upper layers for
 * further processing and visualization. */
void modesMessage::updatePlanes() {
    if (!Modes.stats && (Modes.check_crc == 0 || this->crcok)) {
        /* Track aircrafts in interactive mode or if the HTTP
         * interface is enabled. */
        if (Modes.interactive || Modes.stat_http_requests > 0 || Modes.stat_sbs_connections > 0) {
            struct aircraft *a = this->interactiveReceiveData();
            if (a && Modes.stat_sbs_connections > 0) modesSendSBSOutput(this, a);  /* Feed SBS bitf_output clients. */
        }
        /* In non-interactive way, display messages on standard bitf_output. */
        if (!Modes.interactive) {
            displayModesMessage(this);
            if (!Modes.raw && !Modes.onlyaddr) printf("\n");
        }
        /* Send data to connected clients. */
        if (Modes.net) {
            modesSendRawOutput(this);  /* Feed raw bitf_output clients. */
        }
    }
}

/* Receive new messages and populate the interactive mode with more info. */
aircraft *modesMessage::interactiveReceiveData() {
    uint32_t addr;
    aircraft *a, *aux;

    if (Modes.check_crc && this->crcok == 0) return nullptr;
    addr = (this->aa1 << 16) | (this->aa2 << 8) | this->aa3;

    /* Loookup our aircraft or create a new one. */
    a = interactiveFindAircraft(addr);
    if (!a) {
        a = interactiveCreateAircraft(addr);
        Modes.aircrafts.insert(std::pair<int, aircraft *>(addr, a));
    }

    a->seen = time(NULL);
    a->messages++;

    if (this->msgtype == 0 || this->msgtype == 4 || this->msgtype == 20) {
        a->altitude = this->altitude;
    } else if (this->msgtype == 17) {
        if (this->metype >= 1 && this->metype <= 4) {
            memcpy(a->flight, this->flight, sizeof(a->flight));
        } else if (this->metype >= 9 && this->metype <= 18) {
            a->altitude = this->altitude;
            if (this->fflag) {
                a->odd_cprlat = this->raw_latitude;
                a->odd_cprlon = this->raw_longitude;
                a->odd_cprtime = mstime();
            } else {
                a->even_cprlat = this->raw_latitude;
                a->even_cprlon = this->raw_longitude;
                a->even_cprtime = mstime();
            }
            /* If the two data is less than 10 seconds apart, compute
             * the position. */
            if (abs(a->even_cprtime - a->odd_cprtime) <= 10000) {
                a->decodeCPR();
            }
        } else if (this->metype == 19) {
            if (this->mesub == 1 || this->mesub == 2) {
                a->speed = this->velocity;
                a->track = this->heading;
            }
        }
    }
    return a;
}

/* If the bitf_message type has the checksum xored with the ICAO address, try to
 * brute force it using a list of recently seen ICAO addresses.
 *
 * Do this in a brute-force fashion by xoring the predicted CRC with
 * the address XOR checksum field in the bitf_message. This will recover the
 * address: if we found it in our cache, we can assume the bitf_message is ok.
 *
 * This function expects mm->msgtype and mm->msgbits to be correctly
 * populated by the caller.
 *
 * On success the correct ICAO address is stored in the modesMessage
 * structure in the aa3, aa2, and aa1 fiedls.
 *
 * If the function successfully recovers a bitf_message with a correct checksum
 * it returns 1. Otherwise 0 is returned. */
int modesMessage::bruteForceAP(unsigned char *msg) {
    unsigned char aux[MODES_LONG_MSG_BYTES];
    int msgtype = this->msgtype;
    int msgbits = this->msgbits;

    if (msgtype == 0 ||         /* Short air surveillance */
        msgtype == 4 ||         /* Surveillance, altitude reply */
        msgtype == 5 ||         /* Surveillance, identity reply */
        msgtype == 16 ||        /* Long Air-Air survillance */
        msgtype == 20 ||        /* Comm-A, altitude request */
        msgtype == 21 ||        /* Comm-A, identity request */
        msgtype == 24)          /* Comm-C ELM */
    {
        uint32_t addr;
        uint32_t crc;
        int lastbyte = (msgbits / 8) - 1;

        /* Work on a copy. */
        memcpy(aux, msg, msgbits / 8);

        /* Compute the CRC of the bitf_message and XOR it with the AP field
         * so that we recover the address, because:
         *
         * (ADDR xor CRC) xor CRC = ADDR. */
        crc = modesChecksum(aux, msgbits);
        aux[lastbyte] ^= crc & 0xff;
        aux[lastbyte - 1] ^= (crc >> 8) & 0xff;
        aux[lastbyte - 2] ^= (crc >> 16) & 0xff;

        /* If the obtained address exists in our cache we consider
         * the bitf_message valid. */
        addr = aux[lastbyte] | (aux[lastbyte - 1] << 8) | (aux[lastbyte - 2] << 16);
        if (ICAOAddressWasRecentlySeen(addr)) {
            this->aa1 = aux[lastbyte - 2];
            this->aa2 = aux[lastbyte - 1];
            this->aa3 = aux[lastbyte];
            return 1;
        }
    }
    return 0;
}

/* Hash the ICAO address to index our cache of MODES_ICAO_CACHE_LEN
 * elements, that is assumed to be a power of two. */
uint32_t modesMessage::ICAOCacheHashAddress(uint32_t a) {
    /* The following three rounds will make sure that every bit affects
    * every bitf_output bit with ~ 50% of probability. */
    a = ((a >> 16) ^ a) * 0x45d9f3b;
    a = ((a >> 16) ^ a) * 0x45d9f3b;
    a = ((a >> 16) ^ a);
    return a & (MODES_ICAO_CACHE_LEN - 1);
}

/* Add the specified entry to the cache of recently seen ICAO addresses.
 * Note that we also add a timestamp so that we can make sure that the
 * entry is only valid for MODES_ICAO_CACHE_TTL seconds. */
void modesMessage::addRecentlySeenICAOAddr(uint32_t addr) {
    uint32_t h = this->ICAOCacheHashAddress(addr);
    Modes.icao_cache[h * 2] = addr;
    Modes.icao_cache[h * 2 + 1] = (uint32_t) time(NULL);
}


/* Returns 1 if the specified ICAO address was seen in a DF format with
 * proper checksum (not xored with address) no more than * MODES_ICAO_CACHE_TTL
 * seconds ago. Otherwise returns 0. */
int modesMessage::ICAOAddressWasRecentlySeen(uint32_t addr) {
    uint32_t h = this->ICAOCacheHashAddress(addr);
    uint32_t a = Modes.icao_cache[h * 2];
    uint32_t t = Modes.icao_cache[h * 2 + 1];

    return a && a == addr && time(NULL) - t <= MODES_ICAO_CACHE_TTL;
}

