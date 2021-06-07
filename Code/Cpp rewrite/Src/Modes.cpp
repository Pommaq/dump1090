//
// Created by timmy on 2021-06-06.
//
#include <iostream>
#include <cstring>
#include "../Headers/Modes.hpp"
#include "../Headers/Networking.hpp"
#include "../Headers/Terminal.hpp"




g_settings Modes;



int g_settings::modesInit() {
    int i, q;

    /* We add a full bitf_message minus a final bit to the length, so that we
     * can carry the remaining part of the buffer that we can't process
     * in the bitf_message detection loop, back at the start of the next data
     * to process. This way we are able to also detect messages crossing
     * two reads. */
    this->data_len = MODES_DATA_LEN + (MODES_FULL_LEN - 1) * 4;
    this->data_ready = 0;
    /* Allocate the ICAO address cache. We use two uint32_t for every
     * entry because it's a addr / timestamp pair for every entry. */
    this->icao_cache = new uint32_t[MODES_ICAO_CACHE_LEN*2];
    memset(this->icao_cache, 0, sizeof(uint32_t) * MODES_ICAO_CACHE_LEN * 2);
    this->interactive_last_update = 0;
    try {
        this->data = new unsigned char[this->data_len];
        this->magnitude = new uint16_t[this->data_len];
    }
    catch (std::bad_alloc){
        std::cerr << "Out of memory allocating data buffer." << std::endl;
        return(1);
    }
    memset(this->data, 127, this->data_len);
    /* Populate the I/Q -> Magnitude lookup table. It is used because
     * sqrt or round may be expensive and may vary a lot depending on
     * the libc used.
     *
     * We scale to 0-255 range multiplying by 1.4 in order to ensure that
     * every different I/Q pair will result in a different magnitude value,
     * not losing any resolution. */
    this->maglut = new uint16_t[129*129];
            //malloc(129 * 129 * 2);
    for (i = 0; i <= 128; i++) {
        for (q = 0; q <= 128; q++) {
            this->maglut[i * 129 + q] = round(sqrt(i * i + q * q) * 360);
        }
    }

    /* Statistics */
    this->stat_valid_preamble = 0;
    this->stat_demodulated = 0;
    this->stat_goodcrc = 0;
    this->stat_badcrc = 0;
    this->stat_fixed = 0;
    this->stat_single_bit_fix = 0;
    this->stat_two_bits_fix = 0;
    this->stat_http_requests = 0;
    this->stat_sbs_connections = 0;
    this->stat_out_of_phase = 0;
    this->exit = false;
    return 0;
}

g_settings::g_settings() {
    /* Mutex to synchronize buffer access. */
    this->data_lock = new std::unique_lock<std::mutex>(mtx);

    /* Set sane defaults. */
    this->gain = MODES_MAX_GAIN;
    this->dev_index = 0;
    this->enable_agc = 0;
    this->freq = MODES_DEFAULT_FREQ;
    this->filename = "";
    this->fix_errors = 1;
    this->check_crc = 1;
    this->raw = 0;
    this->net = 0;
    this->net_only = 0;
    this->onlyaddr = 0;
    this->debug = 0;
    this->interactive = 0;
    this->interactive_rows = MODES_INTERACTIVE_ROWS;
    this->interactive_ttl = MODES_INTERACTIVE_TTL;
    this->aggressive = 0;
    this->interactive_rows = getTermRows();
    this->loop = 0;
    this->html_file = P_FILE_GMAP;
}

g_settings::~g_settings() {
    delete this->data_lock;
}


