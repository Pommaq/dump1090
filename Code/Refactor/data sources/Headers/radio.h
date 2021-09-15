//
// Created by timmy on 9/14/21.
//

#ifndef DUMP1090_RADIO_H
#define DUMP1090_RADIO_H
#include "data_source.h"
#include "rtlsdr.h"
#include "rtl-sdr.h"


struct NoDevicesError: public NoSourceException {
    [[nodiscard]] const char * what () const noexcept override
    {
        return "Failed to allocate data source";
    }
};

class radio: virtual protected data_source {
    rtlsdr device;
    radio(rtlsdr&& device, int gain, bool enable_agc, long long int freq, int sample_rate);
    bool fill_buffer() override;
};


#endif //DUMP1090_RADIO_H
