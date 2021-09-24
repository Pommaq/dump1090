#include "radio.h"
#include "rtlsdr.h"

template<std::ptrdiff_t T>
Radio::radio<T>::radio(RTLsdr::rtlsdr<T> &&device, int gain, bool enable_agc, long long int freq, int sample_rate, uint32_t bufnum,
                       uint32_t buffer_length, void *ctx) {
    /*
     * \param bufnum optional buffer count, bufnum * buffer_length = overall buffer size
     *		  set to 0 for default buffer count (15)
     * \param buffer_length optional, must be multiple of 512,
     *		  should be a multiple of 16384 (URB size),
     *		  set to 0 for default buffer length (16 * 32 * 512)
     * \param ctx user specific context to pass to the callback function
     *
     *  I am not sure what these parameters mean, but I am assuming the sdr keeps a pool of <bufnum> buffers
     *  of length buffer_length each. Once one buffer is filled it will be passed to the callback function.
     *
     * throws SourceException upon hitting exceptions
     * */

    this->device = std::move(device);

    this->bufnum = bufnum;
    this->buffer_length = buffer_length;
    this->ctx = ctx;


    bool manual_gain_control = (gain != RTLsdr::settings::MODES_AUTO_GAIN);
    this->device.set_gain_mode(manual_gain_control);

    if (manual_gain_control) {
        if (gain == RTLsdr::settings::MODES_MAX_GAIN) {
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

template<std::ptrdiff_t T>
std::vector<unsigned char> Radio::radio<T>::get_data() {
    return this->device.fill_buffer();
}

template<std::ptrdiff_t T>
void Radio::radio<T>::run() {
    this->device.start(this->bufnum, this->buffer_length, this->cbx);

}
