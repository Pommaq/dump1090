#include "rtlsdr.h"
#include "DataSource.h"

template<std::ptrdiff_t T>
RTLsdr::rtlsdr<T>::rtlsdr(int device_index) {
    this->device = nullptr;

    uint32_t device_count = rtlsdr_get_device_count();
    if (!device_count) {
        throw datasource::SourceException("No SDR devices found");
    }

    rtlsdr_get_device_usb_strings(device_index, this->vendor.data(), this->product.data(), this->serial.data());

    if (rtlsdr_open(&(this->device), device_index) < 0) {
        throw datasource::SourceException("Unable to open SDR device");
    }
}

template<std::ptrdiff_t T>
RTLsdr::rtlsdr<T>::~rtlsdr() {
    if (this->device) {
        try {
            this->kill();
        }
        catch (datasource::SourceException &e) {
            // pass
        }

        rtlsdr_close(this->device);
    }
}

template<std::ptrdiff_t T>
RTLsdr::rtlsdr<T>::rtlsdr(RTLsdr::rtlsdr<T> &&other) noexcept : device(other.device) {
    other.device = nullptr;
}

template<std::ptrdiff_t T>
RTLsdr::rtlsdr<T> &RTLsdr::rtlsdr<T>::operator=(RTLsdr::rtlsdr<T> &&other) noexcept {
    this->device = other.device;
    other.device = nullptr;
    return *this;
}

template<std::ptrdiff_t T>
void RTLsdr::rtlsdr<T>::set_gain_mode(bool manual) {
    rtlsdr_set_tuner_gain_mode(this->device, manual);
}

template<std::ptrdiff_t T>
int RTLsdr::rtlsdr<T>::get_tuner_gains(std::array<int, 100> &buffer) {
    /* Given buffer state will be undefined if SDRAttributeError is thrown */
    int numgain = rtlsdr_get_tuner_gains(this->device, buffer.data());
    if (!numgain) {
        throw datasource::SourceException("Unable to get tuner gain levels");
    }
    return numgain;
}

template<std::ptrdiff_t T>
void RTLsdr::rtlsdr<T>::set_tuner_gain(int gain) {
    int cant_set_gain = rtlsdr_set_tuner_gain(this->device, gain);
    if (cant_set_gain) {
        throw datasource::SourceException("Unable to set tuner gain level");
    }
}

template<std::ptrdiff_t T>
void RTLsdr::rtlsdr<T>::set_freq_correction(int ppm) {
    int success = rtlsdr_set_freq_correction(this->device, ppm);
    if (!success) {
        throw datasource::SourceException("Unable to set frequency correction");
    }
}

template<std::ptrdiff_t T>
void RTLsdr::rtlsdr<T>::set_agc_mode(bool enabled) {
    int success = rtlsdr_set_agc_mode(this->device, enabled);
    if (!success) {
        throw datasource::SourceException("Unable to set AGC mode");
    }
}

template<std::ptrdiff_t T>
void RTLsdr::rtlsdr<T>::set_center_freq(long long int freq) {
    int success = rtlsdr_set_center_freq(this->device, freq);
    if (!success) {
        throw datasource::SourceException("Unable to set center frequency");
    }
}

template<std::ptrdiff_t T>
void RTLsdr::rtlsdr<T>::set_sample_rate(int sample_rate) {
    int success = rtlsdr_set_sample_rate(this->device, sample_rate);

    if (success != 0) {
        if (success == -EINVAL) {
            // The value is in a bad range
            throw datasource::SourceException("Sample rate has invalid value");
        }
        throw datasource::SourceException("Unable to set device sampling rate");
    }
}

template<std::ptrdiff_t T>
void RTLsdr::rtlsdr<T>::reset_buffer() {
    rtlsdr_reset_buffer(this->device);
}

template<std::ptrdiff_t T>
void RTLsdr::rtlsdr<T>::callback(unsigned char *buf, uint32_t len, void *) {
    std::vector<unsigned char> tmp(len);

    for (uint32_t i = 0; i < len; i++) {
        tmp.push_back(buf[i]);
    }

    std::unique_lock scope(this->buffer_mtx);
    this->buffered_data.emplace_back(std::move(tmp));
    this->data_available.release(1);
}

template<std::ptrdiff_t T>
std::vector<unsigned char> RTLsdr::rtlsdr<T>::fill_buffer() {
    this->data_available.acquire();
    std::unique_lock scope(this->buffer_mtx);
    // Get the data away from the list so we can unlock the mutex.
    std::vector<unsigned char> to_return = this->buffered_data.front();
    this->buffered_data.pop_front();

    return to_return;
}

template<std::ptrdiff_t T>
void RTLsdr::rtlsdr<T>::start(uint32_t bufnum, uint32_t buffer_length, void* cbx) {
    this->reader_handle = std::thread(this->threadEntryPoint, cbx, bufnum, buffer_length);
}

template<std::ptrdiff_t T>
void RTLsdr::rtlsdr<T>::threadEntryPoint(void *ctx, uint32_t bufnum, uint32_t buffer_length) {
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
    if (rtlsdr_read_async(this->device,
                      this->callback,
                      ctx,
                      bufnum,
                      buffer_length
    )) {
        throw datasource::SourceException("Failed starting device in asynchronous mode");
    }

}

template<std::ptrdiff_t T>
void RTLsdr::rtlsdr<T>::kill() {
    if (!rtlsdr_cancel_async(this->device)) {
        throw datasource::SourceException("Failed canceling device operations");
    }
    this->reader_handle.join();
}
