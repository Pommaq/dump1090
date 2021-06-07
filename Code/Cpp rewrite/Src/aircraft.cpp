//
// Created by timmy on 2021-06-06.
//

#include <cstring>
#include "../Headers/aircraft.hpp"
#include "../Headers/Modes.hpp"
#include "../Headers/Decoding.hpp"
#include "../Headers/Utilities.hpp"

/* Return a description of planes in json. */
char *aircraftsToJson(int *len) {
    aircraft *a = Modes.aircrafts;
    int buflen = 1024; /* The initial buffer is incremented as needed. */
    char *buf = new char[buflen];
    char *p = buf;
    int l;

    l = snprintf(p, buflen, "[\n");
    p += l;
    buflen -= l;
    while (a) {
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
        a = a->next;
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
    struct aircraft *a = Modes.aircrafts;
    struct aircraft *prev = NULL;
    time_t now = time(NULL);

    while (a) {
        if ((now - a->seen) > Modes.interactive_ttl) {
            struct aircraft *next = a->next;
            /* Remove the element from the linked list, with care
             * if we are removing the first element. */
            free(a);
            if (!prev)
                Modes.aircrafts = next;
            else
                prev->next = next;
            a = next;
        } else {
            prev = a;
            a = a->next;
        }
    }
}


/* Receive new messages and populate the interactive mode with more info. */
aircraft *interactiveReceiveData(modesMessage *mm) {
    uint32_t addr;
    aircraft *a, *aux;

    if (Modes.check_crc && mm->crcok == 0) return nullptr;
    addr = (mm->aa1 << 16) | (mm->aa2 << 8) | mm->aa3;

    /* Loookup our aircraft or create a new one. */
    a = interactiveFindAircraft(addr);
    if (!a) {
        a = interactiveCreateAircraft(addr);
        a->next = Modes.aircrafts;
        Modes.aircrafts = a;
    } else {
        /* If it is an already known aircraft, move it on head
         * so we keep aircrafts ordered by received bitf_message time.
         *
         * However move it on head only if at least one second elapsed
         * since the aircraft that is currently on head sent a bitf_message,
         * othewise with multiple aircrafts at the same time we have an
         * useless shuffle of positions on the screen. */
        if (0 && Modes.aircrafts != a && (time(NULL) - a->seen) >= 1) { // TODO: My IDE reports this as always false. Investigate.
            aux = Modes.aircrafts;
            while (aux->next != a) aux = aux->next;
            /* Now we are a node before the aircraft to remove. */
            aux->next = aux->next->next; /* removed. */
            /* Add on head */
            a->next = Modes.aircrafts;
            Modes.aircrafts = a;
        }
    }

    a->seen = time(NULL);
    a->messages++;

    if (mm->msgtype == 0 || mm->msgtype == 4 || mm->msgtype == 20) {
        a->altitude = mm->altitude;
    } else if (mm->msgtype == 17) {
        if (mm->metype >= 1 && mm->metype <= 4) {
            memcpy(a->flight, mm->flight, sizeof(a->flight));
        } else if (mm->metype >= 9 && mm->metype <= 18) {
            a->altitude = mm->altitude;
            if (mm->fflag) {
                a->odd_cprlat = mm->raw_latitude;
                a->odd_cprlon = mm->raw_longitude;
                a->odd_cprtime = mstime();
            } else {
                a->even_cprlat = mm->raw_latitude;
                a->even_cprlon = mm->raw_longitude;
                a->even_cprtime = mstime();
            }
            /* If the two data is less than 10 seconds apart, compute
             * the position. */
            if (abs(a->even_cprtime - a->odd_cprtime) <= 10000) {
                decodeCPR(a);
            }
        } else if (mm->metype == 19) {
            if (mm->mesub == 1 || mm->mesub == 2) {
                a->speed = mm->velocity;
                a->track = mm->heading;
            }
        }
    }
    return a;
}


/* Return the aircraft with the specified address, or NULL if no aircraft
 * exists with this address. */
aircraft *interactiveFindAircraft(uint32_t addr) {
    struct aircraft *a = Modes.aircrafts;

    while (a) {
        if (a->addr == addr) return a;
        a = a->next;
    }
    return nullptr;
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





