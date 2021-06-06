//
// Created by timmy on 2021-06-06.
//

#ifndef DUMP1090_AIRCRAFT_HPP
#define DUMP1090_AIRCRAFT_HPP

#include <cstdint>
#include <ctime>

/* Structure used to describe an aircraft in iteractive mode. */
struct aircraft {
    uint32_t addr;      /* ICAO address */
    char hexaddr[7];    /* Printable ICAO address */
    char flight[9];     /* Flight number */
    int altitude;       /* Altitude */
    int speed;          /* Velocity computed from EW and NS components. */
    int track;          /* Angle of flight. */
    time_t seen;        /* Time at which the last packet was received. */
    long messages;      /* Number of Mode S messages received. */
    /* Encoded latitude and longitude as extracted by odd and even
     * CPR encoded messages. */
    int odd_cprlat;
    int odd_cprlon;
    int even_cprlat;
    int even_cprlon;
    double lat, lon;    /* Coordinated obtained from CPR encoded data. */
    long long odd_cprtime, even_cprtime;
    struct aircraft *next; /* Next aircraft in our linked list. */
};

void interactiveRemoveStaleAircrafts();

char *aircraftsToJson(int *len);

aircraft *interactiveReceiveData(struct modesMessage *mm);

aircraft *interactiveCreateAircraft(uint32_t addr);

aircraft *interactiveFindAircraft(uint32_t addr);

#endif //DUMP1090_AIRCRAFT_HPP
