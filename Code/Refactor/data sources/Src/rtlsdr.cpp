//
// Created by timmy on 9/15/21.
//

#include "rtlsdr.h"

rtlsdr::rtlsdr(int device_index) {
    this->device = nullptr;

    uint32_t device_count = rtlsdr_get_device_count();
    if (!device_count) {
        throw NoSdrException();
    }

    rtlsdr_get_device_usb_strings(device_index, this->vendor.data(), this->product.data(), this->serial.data());

    if (rtlsdr_open(&(this->device), device_index) < 0) {
        throw BadSdrException();
    }
}

rtlsdr::~rtlsdr() {
    if (this->device) {
        rtlsdr_close(this->device);
    }
}

rtlsdr::rtlsdr(rtlsdr &&other) noexcept : device(other.device), vendor(other.vendor), product(other.product), serial(other.serial)  {
    other.device = nullptr;
}

rtlsdr &rtlsdr::operator=(rtlsdr &&other)  noexcept {
    this->device = other.device;
    other.device = nullptr;
    this->vendor = other.vendor;
    this->product = other.product;
    this->serial = other.serial;
    return *this;
}

void rtlsdr::set_gain_mode(bool manual) {
    rtlsdr_set_tuner_gain_mode(this->device, manual);
}

int rtlsdr::get_tuner_gains(std::array<int, 100> &buffer) {
    /* Given buffer state will be undefined if SDRException is thrown */
    int numgain = rtlsdr_get_tuner_gains(this->device, buffer.data());
    if (!numgain) {
        throw SDRAttributeError();
    }
    return numgain;
}

void rtlsdr::set_tuner_gain(int gain) {
    int cant_set_gain = rtlsdr_set_tuner_gain(this->device, gain);
    if (cant_set_gain) {
        throw SDRAttributeError();
    }
}

void rtlsdr::set_freq_correction(int ppm) {
    int success = rtlsdr_set_freq_correction(this->device,ppm);
    if (!success) {
        throw SDRAttributeError();
    }
}

void rtlsdr::set_agc_mode(bool enabled) {
    int success = rtlsdr_set_agc_mode(this->device, enabled);
    if (!success) {
        throw SDRAttributeError();
    }
}

void rtlsdr::set_center_freq(long long int freq) {
    int success = rtlsdr_set_center_freq(this->device, freq);
    if (!success) {
        throw SDRAttributeError();
    }
}

void rtlsdr::set_sample_rate(int sample_rate) {
    int success = rtlsdr_set_sample_rate(this->device, sample_rate);
    // TODO: if success == -EINVAL, log that the sample rate is invalid.
    if (!success) {
        throw SDRAttributeError();
    }
}

void rtlsdr::reset_buffer() {
    rtlsdr_reset_buffer(this->device);
}



