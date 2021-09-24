#ifndef DUMP1090_RADIO_H
#define DUMP1090_RADIO_H

#include "data_source.h"
#include "rtlsdr.h"
#include <concepts>

template <typename T>
namespace concepts {
    concept Radio = requires(T v) {
        {v.start(uint32_t bufnum, uint32_t buffer_length, void* cbx = nullptr)} -> void;

        // Stuff required to set settings
        {set_gain_mode(bool manual)} -> void;
        {set_tuner_gain(std::array<int, 100> &buffer)} -> int;
        {set_freq_correction(int ppm)} -> void;
        {set_agc_mode(bool enabled)} -> void;
        {set_center_freq(long long int freq)} -> void;
        {set_sample_rate(int sample_rate)} -> void;
        {reset_buffer()} -> void;
    };
}

/* Responsible for setting up the given device with sane default values. */
template<std::ptrdiff_t T>
requires concepts::Radio || std::move_constructible
class radio : virtual protected data_source {
private:
    rtlsdr<T> device;
    uint32_t bufnum;
    uint32_t buffer_length;
    void* ctx;
public:
    radio(rtlsdr<T> &&device, int gain, bool enable_agc, long long int freq, int sample_rate, uint32_t bufnum,
          uint32_t buffer_length, void *ctx);

    void run();
    std::vector<unsigned char> get_data() override;
};


#endif //DUMP1090_RADIO_H
