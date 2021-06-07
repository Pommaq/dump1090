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

/* This algorithm comes from:
 * http://www.lll.lu/~edward/edward/adsb/DecodingADSBposition.html.
 *
 *
 * A few remarks:
 * 1) 131072 is 2^17 since CPR latitude and longitude are encoded in 17 bits.
 * 2) We assume that we always received the odd packet as last packet for
 *    simplicity. This may provide a position that is less fresh of a few
 *    seconds.
 */
void aircraft::decodeCPR() {
    const double AirDlat0 = 360.0 / 60;
    const double AirDlat1 = 360.0 / 59;
    double lat0 = this->even_cprlat;
    double lat1 = this->odd_cprlat;
    double lon0 = this->even_cprlon;
    double lon1 = this->odd_cprlon;

    /* Compute the Latitude Index "j" */
    int j = floor(((59 * lat0 - 60 * lat1) / 131072) + 0.5);
    double rlat0 = AirDlat0 * (cprModFunction(j, 60) + lat0 / 131072);
    double rlat1 = AirDlat1 * (cprModFunction(j, 59) + lat1 / 131072);

    if (rlat0 >= 270) rlat0 -= 360;
    if (rlat1 >= 270) rlat1 -= 360;

    /* Check that both are in the same latitude zone, or abort. */
    if (cprNLFunction(rlat0) != cprNLFunction(rlat1)) return;

    /* Compute ni and the longitude index m */
    if (this->even_cprtime > this->odd_cprtime) {
        /* Use even packet. */
        int ni = cprNFunction(rlat0, 0);
        int m = floor((((lon0 * (cprNLFunction(rlat0) - 1)) -
                        (lon1 * cprNLFunction(rlat0))) / 131072) + 0.5);
        this->lon = cprDlonFunction(rlat0, 0) * (cprModFunction(m, ni) + lon0 / 131072);
        this->lat = rlat0;
    } else {
        /* Use odd packet. */
        int ni = cprNFunction(rlat1, 1);
        int m = floor((((lon0 * (cprNLFunction(rlat1) - 1)) -
                        (lon1 * cprNLFunction(rlat1))) / 131072.0) + 0.5);
        this->lon = cprDlonFunction(rlat1, 1) * (cprModFunction(m, ni) + lon1 / 131072);
        this->lat = rlat1;
    }
    if (this->lon > 180) this->lon -= 360;
}

int aircraft::cprNLFunction(double lat) {
    if (lat < 0) lat = -lat; /* Table is symmetric about the equator. */
    if (lat < 10.47047130) return 59;
    if (lat < 14.82817437) return 58;
    if (lat < 18.18626357) return 57;
    if (lat < 21.02939493) return 56;
    if (lat < 23.54504487) return 55;
    if (lat < 25.82924707) return 54;
    if (lat < 27.93898710) return 53;
    if (lat < 29.91135686) return 52;
    if (lat < 31.77209708) return 51;
    if (lat < 33.53993436) return 50;
    if (lat < 35.22899598) return 49;
    if (lat < 36.85025108) return 48;
    if (lat < 38.41241892) return 47;
    if (lat < 39.92256684) return 46;
    if (lat < 41.38651832) return 45;
    if (lat < 42.80914012) return 44;
    if (lat < 44.19454951) return 43;
    if (lat < 45.54626723) return 42;
    if (lat < 46.86733252) return 41;
    if (lat < 48.16039128) return 40;
    if (lat < 49.42776439) return 39;
    if (lat < 50.67150166) return 38;
    if (lat < 51.89342469) return 37;
    if (lat < 53.09516153) return 36;
    if (lat < 54.27817472) return 35;
    if (lat < 55.44378444) return 34;
    if (lat < 56.59318756) return 33;
    if (lat < 57.72747354) return 32;
    if (lat < 58.84763776) return 31;
    if (lat < 59.95459277) return 30;
    if (lat < 61.04917774) return 29;
    if (lat < 62.13216659) return 28;
    if (lat < 63.20427479) return 27;
    if (lat < 64.26616523) return 26;
    if (lat < 65.31845310) return 25;
    if (lat < 66.36171008) return 24;
    if (lat < 67.39646774) return 23;
    if (lat < 68.42322022) return 22;
    if (lat < 69.44242631) return 21;
    if (lat < 70.45451075) return 20;
    if (lat < 71.45986473) return 19;
    if (lat < 72.45884545) return 18;
    if (lat < 73.45177442) return 17;
    if (lat < 74.43893416) return 16;
    if (lat < 75.42056257) return 15;
    if (lat < 76.39684391) return 14;
    if (lat < 77.36789461) return 13;
    if (lat < 78.33374083) return 12;
    if (lat < 79.29428225) return 11;
    if (lat < 80.24923213) return 10;
    if (lat < 81.19801349) return 9;
    if (lat < 82.13956981) return 8;
    if (lat < 83.07199445) return 7;
    if (lat < 83.99173563) return 6;
    if (lat < 84.89166191) return 5;
    if (lat < 85.75541621) return 4;
    if (lat < 86.53536998) return 3;
    if (lat < 87.00000000) return 2;
    else return 1;
}

int aircraft::cprNFunction(double lat, int isodd) {
    int nl = this->cprNLFunction(lat) - isodd;
    if (nl < 1) nl = 1;
    return nl;
}

double aircraft::cprDlonFunction(double lat, int isodd) {
    return 360.0 / this->cprNFunction(lat, isodd);
}

/* Always positive MOD operation, used for CPR decoding. */
int aircraft::cprModFunction(int a, int b) {
    int res = a % b;
    if (res < 0) res += b;
    return res;
}
