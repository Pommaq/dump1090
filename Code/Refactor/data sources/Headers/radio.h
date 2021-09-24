#ifndef DUMP1090_RADIO_H
#define DUMP1090_RADIO_H

#include "data_source.h"
#include "rtlsdr.h"
#include <concepts>

namespace Radio {

/* Responsible for setting up the given device with sane default values. */
template<std::ptrdiff_t T>
class radio : virtual protected DataSource::data_source {
private:
    RTLsdr::rtlsdr<T> device;
    uint32_t bufnum;
    uint32_t buffer_length;
    void* ctx;
public:
    radio(RTLsdr::rtlsdr<T> &&device, int gain, bool enable_agc, long long int freq, int sample_rate, uint32_t bufnum,
          uint32_t buffer_length, void *ctx);

    void run();
    std::vector<unsigned char> get_data() override;
};
}


#endif //DUMP1090_RADIO_H
