//
// Created by timmy on 2021-06-06.
//

#ifndef DUMP1090_MODES_HPP
#define DUMP1090_MODES_HPP


#define MODES_DEFAULT_RATE 2000000
#define MODES_DEFAULT_FREQ 1090000000
#define MODES_DEFAULT_WIDTH 1000
#define MODES_DEFAULT_HEIGHT 700
#define MODES_ASYNC_BUF_NUMBER 12
#define MODES_DATA_LEN (16 * 16384) /* 256k */
#define MODES_AUTO_GAIN -100        /* Use automatic gain. */
#define MODES_MAX_GAIN 999999       /* Use max available gain. */

#define MODES_PREAMBLE_US 8 /* microseconds */
#define MODES_LONG_MSG_BITS 112
#define MODES_SHORT_MSG_BITS 56
#define MODES_FULL_LEN (MODES_PREAMBLE_US + MODES_LONG_MSG_BITS)
#define MODES_LONG_MSG_BYTES (112 / 8)
#define MODES_SHORT_MSG_BYTES (56 / 8)

#define MODES_ICAO_CACHE_LEN 1024 /* Power of two required. */
#define MODES_ICAO_CACHE_TTL 60   /* Time to live of cached addresses. */
#define MODES_UNIT_FEET 0
#define MODES_UNIT_METERS 1

#define MODES_DEBUG_DEMOD (1 << 0)
#define MODES_DEBUG_DEMODERR (1 << 1)
#define MODES_DEBUG_BADCRC (1 << 2)
#define MODES_DEBUG_GOODCRC (1 << 3)
#define MODES_DEBUG_NOPREAMBLE (1 << 4)
#define MODES_DEBUG_NET (1 << 5)
#define MODES_DEBUG_JS (1 << 6)

/* When debug is set to MODES_DEBUG_NOPREAMBLE, the first sample must be
 * at least greater than a given level for us to dump the signal. */
#define MODES_DEBUG_NOPREAMBLE_LEVEL 25

#define MODES_INTERACTIVE_REFRESH_TIME 250 /* Milliseconds */
#define MODES_INTERACTIVE_ROWS 15          /* Rows on screen */
#define MODES_INTERACTIVE_TTL 60           /* TTL before being removed */

#define MODES_NET_MAX_FD 1024
#define MODES_NET_OUTPUT_SBS_PORT 30003
#define MODES_NET_OUTPUT_RAW_PORT 30002
#define MODES_NET_INPUT_RAW_PORT 30001
#define MODES_NET_HTTP_PORT 8080
#define MODES_CLIENT_BUF_SIZE 1024
#define MODES_NET_SNDBUF_SIZE (1024 * 64)

#define MODES_NOTUSED(V) ((void)V)

#include <thread>
#include <mutex>
#include <condition_variable>
#include <fstream>
#include <rtl-sdr.h>
#include "anet.h"
#include "data_reader.hpp"


/* =============================== Initialization =========================== */

void modesInitConfig(void);
void modesInit(void);

class g_settings {
public:
    g_settings() = default;

    /* Internal state, intended to be shared among threads */
    std::thread reader_thread;
    std::unique_lock<std::mutex> data_lock;     /* Mutex to synchronize buffer access. */
    std::condition_variable data_cond;       /* Will cause threads to block when 0 and start when "upped" */
    unsigned char *data;            /* Raw IQ samples buffer */
    uint16_t *magnitude;            /* Magnitude vector */
    uint32_t data_len;              /* Buffer length. */
    int fd;                         /* --ifile option file descriptor. */
    std::ifstream ifs;
    int data_ready;                 /* Data ready to be processed. */
    uint32_t *icao_cache;           /* Recently seen ICAO addresses cache. */
    uint16_t *maglut;               /* I/Q -> Magnitude lookup table. */
    volatile bool exit;                       /* Exit from the main loop when true. */

    /* RTLSDR */
    int dev_index;
    double gain;
    int enable_agc;
    rtlsdr_dev_t *dev;
    long long int freq;

    /* Networking */
    char aneterr[ANET_ERR_LEN];
    struct client *clients[MODES_NET_MAX_FD]; /* Our clients. */
    int maxfd;                      /* Greatest fd currently active. */
    int sbsos;                      /* SBS bitf_output listening socket. */
    int ros;                        /* Raw bitf_output listening socket. */
    int ris;                        /* Raw input listening socket. */
    int https;                      /* HTTP listening socket. */

    /* Configuration */
    std::string filename;                 /* Input form file, --ifile option. */
    std::string html_file;                /* HTML file to load, --html_file, defaults to P_FILE_GMAP"*/
    int loop;                       /* Read input file again and again. */
    int fix_errors;                 /* Single bit error correction if true. */
    int check_crc;                  /* Only display messages with good CRC. */
    int raw;                        /* Raw bitf_output format. */
    int debug;                      /* Debugging mode. */
    int net;                        /* Enable networking. */
    int net_only;                   /* Enable just networking. */
    int interactive;                /* Interactive mode */
    int interactive_rows;           /* Interactive mode: max number of rows. */
    int interactive_ttl;            /* Interactive mode: TTL before deletion. */
    int stats;                      /* Print stats at exit in --ifile mode. */
    int onlyaddr;                   /* Print only ICAO addresses. */
    int metric;                     /* Use metric units. */
    int aggressive;                 /* Aggressive detection algorithm. */

    /* Interactive mode */
    struct aircraft *aircrafts;
    long long interactive_last_update;  /* Last screen update in milliseconds */

    /* Statistics */
    long long stat_valid_preamble;
    long long stat_demodulated;
    long long stat_goodcrc;
    long long stat_badcrc;
    long long stat_fixed;
    long long stat_single_bit_fix;
    long long stat_two_bits_fix;
    long long stat_http_requests;
    long long stat_sbs_connections;
    long long stat_out_of_phase;
};

extern g_settings Modes;

void modesInitConfig();
void modesInit(void);

#endif //DUMP1090_MODES_HPP

