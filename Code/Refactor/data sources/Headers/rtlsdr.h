//
// Created by timmy on 9/15/21.
//

#ifndef DUMP1090_RTLSDR_H
#define DUMP1090_RTLSDR_H

#include <array>
#include <list>
#include <thread>
#include <mutex>
#include <semaphore>
#include <vector>

#include "rtl-sdr.h"
#include "data_source.h"

#define DEFAULT_SDR_BUFFER_LENGTH (16384*4)
#define DEFAULT_SDR_BUFFER_COUNT 15
#define MODES_AUTO_GAIN -100        /* Use automatic gain. */
#define MODES_MAX_GAIN 999999       /* Use max available gain. */

/*
 * Class responsible for making sure the sdr handle is allocated and released properly.
 */
template<std::ptrdiff_t T>
class rtlsdr {
private:
    rtlsdr_dev_t *device;
    std::array<char, 256> vendor{0}, product{0}, serial{0};

    std::mutex buffer_mtx;
    std::counting_semaphore<T> data_available;
    std::thread reader_handle;

    void callback(unsigned char *buf, uint32_t len, void *);

    void threadEntryPoint(void *ctx = nullptr, uint32_t bufnum = DEFAULT_SDR_BUFFER_COUNT, uint32_t buffer_length = DEFAULT_SDR_BUFFER_LENGTH);

    std::list<std::vector<unsigned char>> buffered_data;

public:
    explicit rtlsdr(int device_index = 0);

    /* We only permit moving to avoid closing handles that are in use. */
    rtlsdr(rtlsdr &&other) noexcept;

    rtlsdr &operator=(rtlsdr &&other) noexcept;


    rtlsdr(rtlsdr &other) = delete;

    rtlsdr &operator=(rtlsdr &other) = delete;

    ~rtlsdr();

    void set_gain_mode(bool manual);

    int get_tuner_gains(std::array<int, 100> &buffer);

    void set_tuner_gain(int gain);

    void set_freq_correction(int ppm);

    void set_agc_mode(bool enabled);

    void set_center_freq(long long int freq);

    void set_sample_rate(int sample_rate);

    void reset_buffer();

    std::vector<unsigned char> fill_buffer();

    void start(uint32_t bufnum, uint32_t buffer_length, void* cbx = nullptr);
    void kill();

};


#endif //DUMP1090_RTLSDR_H
