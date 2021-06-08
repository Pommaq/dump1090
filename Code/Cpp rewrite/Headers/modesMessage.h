//
// Created by timmy on 2021-06-07.
//

#ifndef DUMP1090_MODESMESSAGE_H
#define DUMP1090_MODESMESSAGE_H


#include "aircraft.hpp"
#include "Modes.hpp"

/* The struct we use to store information about a decoded bitf_message. */
struct modesMessage {
private:
    aircraft *interactiveReceiveData();
    int bruteForceAP(unsigned char *msg);
    uint32_t ICAOCacheHashAddress(uint32_t a);
    void addRecentlySeenICAOAddr(uint32_t addr);
    int ICAOAddressWasRecentlySeen(uint32_t addr);
public:
    modesMessage() = default;
    ~modesMessage() = default;
    modesMessage(unsigned char *msg);
    void updatePlanes();
    /* Generic fields */
    unsigned char msg[MODES_LONG_MSG_BYTES+1]; /* Binary bitf_message. */
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



#endif //DUMP1090_MODESMESSAGE_H
