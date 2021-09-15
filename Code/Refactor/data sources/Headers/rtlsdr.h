//
// Created by timmy on 9/15/21.
//

#ifndef DUMP1090_RTLSDR_H
#define DUMP1090_RTLSDR_H
#include "rtl-sdr.h"
#include "data_source.h"
#include <iostream>
#include <array>

#define MODES_AUTO_GAIN -100        /* Use automatic gain. */
#define MODES_MAX_GAIN 999999       /* Use max available gain. */


struct SDRException: public SourceException {
    [[nodiscard]] const char * what () const noexcept override
    {
        return "Something went wrong with the SDR";
    }
};

struct NoSdrException: public SDRException {
    [[nodiscard]] const char * what () const noexcept override
    {
        return "No SDR devices available";
    }
};

struct BadSdrException: public SDRException {
    [[nodiscard]] const char * what () const noexcept override
    {
        return "Error opening SDR device";
    }
};
struct BadRangeError: SDRException {

    [[nodiscard]] const char * what () const noexcept override
    {
        return "Gain outside allowed range";
    }
};

struct SDRAttributeError: SDRException {
    [[nodiscard]] const char * what () const noexcept override
    {
        return "Unable to get/set attribute";
    }
};


/*
 * Class responsible for making sure the sdr handle is allocated and released properly.
 */
class rtlsdr {
private:
    rtlsdr_dev_t *device;
    std::array<char, 256> vendor{}, product{}, serial{};
public:
    explicit rtlsdr(int device_index = 0);
    /* We only permit moving to avoid closing handles that are in use. */
    rtlsdr(rtlsdr&& other) noexcept;
    rtlsdr& operator=(rtlsdr&& other) noexcept;


    rtlsdr(rtlsdr& other) = delete;
    rtlsdr& operator=(rtlsdr& other) = delete;
    ~rtlsdr();

    void set_gain_mode(bool manual);
    int get_tuner_gains(std::array<int, 100> &buffer);
    void set_tuner_gain(int gain);
    void set_freq_correction(int ppm);
    void set_agc_mode(bool enabled);
    void set_center_freq(long long int freq);
    void set_sample_rate(int sample_rate);
    void reset_buffer();

};


#endif //DUMP1090_RTLSDR_H
