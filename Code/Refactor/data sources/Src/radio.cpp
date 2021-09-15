//
// Created by timmy on 9/14/21.
//

#include "radio.h"

/* Throws SDRAttributeError if an attribute cant be set */
radio::radio(rtlsdr&& device, int gain, bool enable_agc, long long int freq, int sample_rate){
    this->device = std::move(device);

    bool manual_gain_control = (gain != MODES_AUTO_GAIN);
    /* Set gain, frequency, sample rate, and reset the device. */
    this->device.set_gain_mode(manual_gain_control);

    if (manual_gain_control) {
        if (gain == MODES_MAX_GAIN) {
            /* Find the maximum gain available. */
            std::array<int, 100> gains{};

            int numgains = this->device.get_tuner_gains(gains);

            gain = gains[numgains - 1];
        }

        this->device.set_tuner_gain(gain);
    }
    this->device.set_freq_correction(0);
    this->device.set_agc_mode(enable_agc);
    this->device.set_center_freq(freq);
    this->device.set_sample_rate(sample_rate);
    this->device.reset_buffer();
}

bool radio::fill_buffer() {
    // TODO: Implement
    return false;
}
