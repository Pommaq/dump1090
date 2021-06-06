//
// Created by timmy on 2021-06-05.
//

#ifndef DUMP1090_DATA_READER_HPP
#define DUMP1090_DATA_READER_HPP

#include "rtl-sdr.h"
#include "networking.hpp"
#include "Modes.hpp"
#include <thread>
#include <mutex>
#include <condition_variable>

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
#define P_FILE_GMAP "/srv/gmap.html" /* Used in networking*/

void modesInitRTLSDR(void);

void rtlsdrCallback(unsigned char *buf, uint32_t len, void *ctx);

void readDataFromFile(void);

void *readerThreadEntryPoint(void *arg);

void modesInit();


void modesInitConfig() {


#endif // DUMP1090_DATA_READER_HPP
