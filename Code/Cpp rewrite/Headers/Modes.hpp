//
// Created by timmy on 2021-06-06.
//

#ifndef DUMP1090_MODES_HPP
#define DUMP1090_MODES_HPP
#include <thread>
#include <mutex>
#include <condition_variable>
#include <fstream>
#include <rtl-sdr.h>
#include "networking.hpp"
#include "data_reader.hpp"

struct g_settings{
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

#endif //DUMP1090_MODES_HPP

