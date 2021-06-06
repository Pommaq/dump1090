//
// Created by timmy on 2021-06-06.
//

#include <csignal>
#include "../Headers/Terminal.hpp"
#include "../Headers/Modes.hpp"
#include "../Headers/aircraft.hpp"
#include <unistd.h>
#include <asm-generic/ioctls.h>
#include <sys/ioctl.h>
#include <cstring>
#include <iostream>

/* ============================ Terminal handling  ========================== */


const char *ca_str[8] = {
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
const char *fs_str[8] = {
        /* 0 */ "Normal, Airborne",
        /* 1 */ "Normal, On the ground",
        /* 2 */ "ALERT,  Airborne",
        /* 3 */ "ALERT,  On the ground",
        /* 4 */ "ALERT & Special Position Identification. Airborne or Ground",
        /* 5 */ "Special Position Identification. Airborne or Ground",
        /* 6 */ "Value 6 is not assigned",
        /* 7 */ "Value 7 is not assigned"
};


/* Handle resizing terminal. */
void sigWinchCallback() {
    signal(SIGWINCH, SIG_IGN);
    Modes.interactive_rows = getTermRows();
    interactiveShowData();
    signal(SIGWINCH, reinterpret_cast<__sighandler_t>(sigWinchCallback));
}

/* Get the number of rows after the terminal changes size. */
int getTermRows() {
    winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return w.ws_row;
}


/* Show the currently captured interactive data on screen. */
void interactiveShowData() {
    struct aircraft *a = Modes.aircrafts;
    time_t now = time(nullptr);
    char progress[4];
    int count = 0;

    memset(progress, ' ', 3);
    progress[time(nullptr) % 3] = '.';
    progress[3] = '\0';

    std::cout << "\x1b[H\x1b[2J"    /* Clear the screen */
              <<
              "Hex    Flight   Altitude  Speed   Lat       Lon       Track  Messages Seen "
              << progress << "\n--------------------------------------------------------------------------------"
              << std::endl;

    while (a && count < Modes.interactive_rows) {
        int altitude = a->altitude, speed = a->speed;

        /* Convert units to metric if --metric was specified. */
        if (Modes.metric) {
            altitude /= 3.2828;
            speed *= 1.852;
        }

        printf("%-6s %-8s %-9d %-7d %-7.03f   %-7.03f   %-3d   %-9ld %d sec\n",
               a->hexaddr, a->flight, altitude, speed,
               a->lat, a->lon, a->track, a->messages,
               (int) (now - a->seen));
        a = a->next;
        count++;
    }
}


/* This function gets a decoded Mode S Message and prints it on the screen
 * in a human readable format. */
void displayModesMessage(modesMessage *mm) {
    int j;

    /* Handle only addresses mode first. */
    if (Modes.onlyaddr) {
        printf("%02x%02x%02x\n", mm->aa1, mm->aa2, mm->aa3);
        return;
    }

    /* Show the raw bitf_message. */
    printf("*");
    for (j = 0; j < mm->msgbits / 8; j++) printf("%02x", mm->msg[j]);
    printf(";\n");

    if (Modes.raw) {
        fflush(stdout); /* Provide data to the reader ASAP. */
        return; /* Enough for --raw mode */
    }

    printf("CRC: %06x (%s)\n", (int) mm->crc, mm->crcok ? "ok" : "wrong");
    if (mm->errorbit != -1)
        std::cout << "Single bit error fixed, bit " << mm->errorbit << std::endl;

    if (mm->msgtype == 0) {
        /* DF 0 */
        std::cout << "DF 0: Short Air-Air Surveillance." << std::endl
        << "  Altitude       : " << mm->altitude
        << ((mm->unit == MODES_UNIT_METERS) ? "meters" : "feet") << std::endl;
        printf("  ICAO Address   : %02x%02x%02x\n", mm->aa1, mm->aa2, mm->aa3);
    } else if (mm->msgtype == 4 || mm->msgtype == 20) {
        std::cout << "DF " << mm->msgtype << ":" << ((mm->msgtype == 4) ? "Surveillance" : "Comm-B") << ", Altitude Reply." << std::endl
        << "  Flight Status  : " << fs_str[mm->fs] << std::endl
        << "  DR             : " << mm->dr << std::endl
        << "  UM             : " << mm->um << std::endl
        << "  Altitude       : " << mm->altitude << " " << ((mm->unit == MODES_UNIT_METERS) ? "meters" : "feet") << std::endl;
        printf("  ICAO Address   : %02x%02x%02x\n", mm->aa1, mm->aa2, mm->aa3);

        if (mm->msgtype == 20) {
            /* TODO: 56 bits DF20 MB additional field. */
        }
    } else if (mm->msgtype == 5 || mm->msgtype == 21) {
        printf("DF %d: %s, Identity Reply.\n", mm->msgtype,
               (mm->msgtype == 5) ? "Surveillance" : "Comm-B");
        printf("  Flight Status  : %s\n", fs_str[mm->fs]);
        printf("  DR             : %d\n", mm->dr);
        printf("  UM             : %d\n", mm->um);
        printf("  Squawk         : %d\n", mm->identity);
        printf("  ICAO Address   : %02x%02x%02x\n", mm->aa1, mm->aa2, mm->aa3);

        if (mm->msgtype == 21) {
            /* TODO: 56 bits DF21 MB additional field. */
        }
    } else if (mm->msgtype == 11) {
        /* DF 11 */
        printf("DF 11: All Call Reply.\n");
        printf("  Capability  : %s\n", ca_str[mm->ca]);
        printf("  ICAO Address: %02x%02x%02x\n", mm->aa1, mm->aa2, mm->aa3);
    } else if (mm->msgtype == 17) {
        /* DF 17 */
        std::cout << "DF 17: ADS-B bitf_message." << std::endl
        << "  Capability     :" << mm->ca << "(" << ca_str[mm->ca] << ")" << std::endl;
        printf("  ICAO Address   : %02x%02x%02x\n", mm->aa1, mm->aa2, mm->aa3);
        std::cout << "  Extended Squitter  Type: " << mm->metype << std::endl
        << "  Extended Squitter  Sub : " << mm->mesub << std::endl
        << "  Extended Squitter  Name: " << getMEDescription(mm->metype, mm->mesub) << std::endl;

        /* Decode the extended squitter bitf_message. */
        if (mm->metype >= 1 && mm->metype <= 4) {
            /* Aircraft identification. */
            char *ac_type_str[4] = {
                    "Aircraft Type D",
                    "Aircraft Type C",
                    "Aircraft Type B",
                    "Aircraft Type A"
            };

            printf("    Aircraft Type  : %s\n", ac_type_str[mm->aircraft_type]);
            printf("    Identification : %s\n", mm->flight);
        } else if (mm->metype >= 9 && mm->metype <= 18) {
            printf("    F flag   : %s\n", mm->fflag ? "odd" : "even");
            printf("    T flag   : %s\n", mm->tflag ? "UTC" : "non-UTC");
            printf("    Altitude : %d feet\n", mm->altitude);
            printf("    Latitude : %d (not decoded)\n", mm->raw_latitude);
            printf("    Longitude: %d (not decoded)\n", mm->raw_longitude);
        } else if (mm->metype == 19 && mm->mesub >= 1 && mm->mesub <= 4) {
            if (mm->mesub == 1 || mm->mesub == 2) {
                /* Velocity */
                printf("    EW direction      : %d\n", mm->ew_dir);
                printf("    EW velocity       : %d\n", mm->ew_velocity);
                printf("    NS direction      : %d\n", mm->ns_dir);
                printf("    NS velocity       : %d\n", mm->ns_velocity);
                printf("    Vertical rate src : %d\n", mm->vert_rate_source);
                printf("    Vertical rate sign: %d\n", mm->vert_rate_sign);
                printf("    Vertical rate     : %d\n", mm->vert_rate);
            } else if (mm->mesub == 3 || mm->mesub == 4) {
                printf("    Heading status: %d", mm->heading_is_valid);
                printf("    Heading: %d", mm->heading);
            }
        } else {
            printf("    Unrecognized ME type: %d subtype: %d\n",
                   mm->metype, mm->mesub);
        }
    } else {
        if (Modes.check_crc)
            printf("DF %d with good CRC received "
                   "(decoding still not implemented).\n",
                   mm->msgtype);
    }
}


std::string & getMEDescription(int metype, int mesub) {
    static std::string mename = "Unknown";

    if (metype >= 1 && metype <= 4)
        mename = "Aircraft Identification and Category";
    else if (metype >= 5 && metype <= 8)
        mename = "Surface Position";
    else if (metype >= 9 && metype <= 18)
        mename = "Airborne Position (Baro Altitude)";
    else if (metype == 19 && mesub >= 1 && mesub <= 4)
        mename = "Airborne Velocity";
    else if (metype >= 20 && metype <= 22)
        mename = "Airborne Position (GNSS Height)";
    else if (metype == 23 && mesub == 0)
        mename = "Test Message";
    else if (metype == 24 && mesub == 1)
        mename = "Surface System Status";
    else if (metype == 28 && mesub == 1)
        mename = "Extended Squitter Aircraft Status (Emergency)";
    else if (metype == 28 && mesub == 2)
        mename = "Extended Squitter Aircraft Status (1090ES TCAS RA)";
    else if (metype == 29 && (mesub == 0 || mesub == 1))
        mename = "Target State and Status Message";
    else if (metype == 31 && (mesub == 0 || mesub == 1))
        mename = "Aircraft Operational Status Message";
    return mename;
}


