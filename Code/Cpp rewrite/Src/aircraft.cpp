//
// Created by timmy on 2021-06-06.
//

#include <cstring>
#include <vector>
#include "../Headers/aircraft.hpp"
#include "../Headers/Modes.hpp"
#include "../Headers/Decoding.hpp"
#include "../Headers/Utilities.hpp"

/* Return a description of planes in json. */
char *aircraftsToJson(int *len) {
    aircraft *a = nullptr;
    int buflen = 1024; /* The initial buffer is incremented as needed. */
    char *buf = new char[buflen];
    char *p = buf;
    int l;

    l = snprintf(p, buflen, "[\n");
    p += l;
    buflen -= l;

    auto it = Modes.aircrafts.begin();
    while (it != Modes.aircrafts.end()) {
        a = it->second;
        int altitude = a->altitude, speed = a->speed;

        /* Convert units to metric if --metric was specified. */
        if (Modes.metric) {
            altitude /= 3.2828;
            speed *= 1.852;
        }

        if (a->lat != 0 && a->lon != 0) {
            l = snprintf(p, buflen,
                         "{\"hex\":\"%s\", \"flight\":\"%s\", \"lat\":%f, "
                         "\"lon\":%f, \"altitude\":%d, \"track\":%d, "
                         "\"speed\":%d},\n",
                         a->hexaddr, a->flight, a->lat, a->lon, a->altitude, a->track,
                         a->speed);
            p += l;
            buflen -= l;
            /* Resize if needed. */
            if (buflen < 256) {
                int used = p - buf;
                buflen += 1024; /* Our increment. */
                buf = static_cast<char*>(realloc(buf, used + buflen));
                p = buf + used;
            }
        }
        it++;
    }
    /* Remove the final comma if any, and closes the json array. */
    if (*(p - 2) == ',') {
        *(p - 2) = '\n';
        p--;
        buflen++;
    }
    l = snprintf(p, buflen, "]\n");
    p += l;
    buflen -= l;

    *len = p - buf;
    return buf;
}


/* When in interactive mode If we don't receive new nessages within
 * MODES_INTERACTIVE_TTL seconds we remove the aircraft from the list. */
void interactiveRemoveStaleAircrafts() {
    struct aircraft *prev = NULL;
    time_t now = time(NULL);

    struct aircraft* a = nullptr;
    std::vector<std::pair<int, aircraft*>> to_erase;
    auto it = Modes.aircrafts.begin();
    while (it != Modes.aircrafts.begin()){
        if ((now - it->second->seen) > Modes.interactive_ttl) {
            /* Remove the element*/
            to_erase.emplace_back(*it);
        }
        it++;
    }
    for (auto &pair: to_erase){
        delete pair.second;
        Modes.aircrafts.erase(pair.first);
    }
}


/* Return the aircraft with the specified address, or NULL if no aircraft
 * exists with this address. */
aircraft *interactiveFindAircraft(uint32_t addr) {
    auto ret =  Modes.aircrafts.find(addr);
    if (ret != Modes.aircrafts.end()){
        return ret->second;
    }
    else{
        return nullptr;
    }

}


/* ========================= Interactive mode =============================== */

/* Return a new aircraft structure for the interactive mode linked list
 * of aircrafts. */
struct aircraft *interactiveCreateAircraft(uint32_t addr) {
    auto *a = new aircraft;

    a->addr = addr;
    snprintf(a->hexaddr, sizeof(a->hexaddr), "%06x", (int) addr);
    a->flight[0] = '\0';
    a->altitude = 0;
    a->speed = 0;
    a->track = 0;
    a->odd_cprlat = 0;
    a->odd_cprlon = 0;
    a->odd_cprtime = 0;
    a->even_cprlat = 0;
    a->even_cprlon = 0;
    a->even_cprtime = 0;
    a->lat = 0;
    a->lon = 0;
    a->seen = time(nullptr);
    a->messages = 0;
    a->next = nullptr;
    return a;
}





