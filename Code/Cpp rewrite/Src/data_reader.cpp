//
// Created by timmy on 2021-06-05.
//
#include "../Headers/data_reader.h"
#include <cstring>
#include <iostream>
/* =============================== RTLSDR handling ========================== */

void modesInitRTLSDR(void) {
  int j;
  int device_count;
  int ppm_error = 0;
  char vendor[256], product[256], serial[256];

  device_count = rtlsdr_get_device_count();
  if (!device_count) {
    std::cerr << "No supported RTLSDR devices found." << std::endl;
    exit(1);
  }
  std::cerr << "Found " << device_count << " device(s):" << std::endl;
  for (j = 0; j < device_count; j++) {
    rtlsdr_get_device_usb_strings(j, vendor, product, serial);
    std::cerr << j << ": " << vendor << "," << product << " SN:" << serial
              << " " << ((j == Modes.dev_index) ? "(currently selected)" : "")
              << std::endl;
  }

  if (rtlsdr_open(&Modes.dev, Modes.dev_index) < 0) {
    std::cerr << "Error opening the RTLSDR device: " << strerror(errno)
              << std::endl;
    exit(1);
  }

  /* Set gain, frequency, sample rate, and reset the device. */
  rtlsdr_set_tuner_gain_mode(Modes.dev,
                             (Modes.gain == MODES_AUTO_GAIN) ? 0 : 1);
  if (Modes.gain != MODES_AUTO_GAIN) {
    if (Modes.gain == MODES_MAX_GAIN) {
      /* Find the maximum gain available. */
      int numgains;
      int gains[100];

      numgains = rtlsdr_get_tuner_gains(Modes.dev, gains);
      Modes.gain = gains[numgains - 1];
      std::cerr << "Max available gain is: " << Modes.gain / 10.0 << std::endl;
    }
    rtlsdr_set_tuner_gain(Modes.dev, Modes.gain);
    std::cerr << "Setting gain to: " << Modes.gain / 10.0;
  } else {
    std::cerr << "Using automatic gain control" << std::endl;
  }
  rtlsdr_set_freq_correction(Modes.dev, ppm_error);
  if (Modes.enable_agc)
    rtlsdr_set_agc_mode(Modes.dev, 1);
  rtlsdr_set_center_freq(Modes.dev, Modes.freq);
  rtlsdr_set_sample_rate(Modes.dev, MODES_DEFAULT_RATE);
  rtlsdr_reset_buffer(Modes.dev);
  std::cerr << "Gain reported by device: "
            << rtlsdr_get_tuner_gain(Modes.dev) / 10.0 << std::endl;
}

/* We use a thread reading data in background, while the main thread
 * handles decoding and visualization of data to the user.
 *
 * The reading thread calls the RTLSDR API to read data asynchronously, and
 * uses a callback to populate the data buffer.
 * A Mutex is used to avoid races with the decoding thread. */
void rtlsdrCallback(unsigned char *buf, uint32_t len, void *ctx) {
  MODES_NOTUSED(ctx);

  pthread_mutex_lock(&Modes.data_mutex);
  if (len > MODES_DATA_LEN)
    len = MODES_DATA_LEN;
  /* Move the last part of the previous buffer, that was not processed,
   * on the start of the new buffer. */
  memcpy(Modes.data, Modes.data + MODES_DATA_LEN, (MODES_FULL_LEN - 1) * 4);
  /* Read the new data. */
  memcpy(Modes.data + (MODES_FULL_LEN - 1) * 4, buf, len);
  Modes.data_ready = 1;
  /* Signal to the other thread that new data is ready */
  pthread_cond_signal(&Modes.data_cond);
  pthread_mutex_unlock(&Modes.data_mutex);
}

/* This is used when --ifile is specified in order to read data from file
 * instead of using an RTLSDR device. */
void readDataFromFile(void) {
  pthread_mutex_lock(&Modes.data_mutex);
  while (true) {
    ssize_t nread, toread;
    unsigned char *p;

    if (Modes.data_ready) {
      pthread_cond_wait(&Modes.data_cond, &Modes.data_mutex);
      continue;
    }

    if (Modes.interactive) {
      /* When --ifile and --interactive are used together, slow down
       * playing at the natural rate of the RTLSDR received. */
      pthread_mutex_unlock(&Modes.data_mutex);
      usleep(5000);
      pthread_mutex_lock(&Modes.data_mutex);
    }

    /* Move the last part of the previous buffer, that was not processed,
     * on the start of the new buffer. */
    memcpy(Modes.data, Modes.data + MODES_DATA_LEN, (MODES_FULL_LEN - 1) * 4);
    toread = MODES_DATA_LEN;
    p = Modes.data + (MODES_FULL_LEN - 1) * 4;
    while (toread) {
      nread = read(Modes.fd, p, toread);
      /* In --file mode, seek the file again from the start
       * and re-play it if --loop was given. */
      if (nread == 0 && Modes.filename != nullptr && Modes.fd != STDIN_FILENO &&
          Modes.loop) {
        if (lseek(Modes.fd, 0, SEEK_SET) != -1)
          continue;
      }

      if (nread <= 0) {
        Modes.exit = 1; /* Signal the other thread to exit. */
        break;
      }
      p += nread;
      toread -= nread;
    }
    if (toread) {
      /* Not enough data on file to fill the buffer? Pad with
       * no signal. */
      memset(p, 127, toread);
    }
    Modes.data_ready = 1;
    /* Signal to the other thread that new data is ready */
    pthread_cond_signal(&Modes.data_cond);
  }
}

/* We read data using a thread, so the main thread only handles decoding
 * without caring about data acquisition. */
void *readerThreadEntryPoint(void *arg) {
  MODES_NOTUSED(arg);

  if (Modes.filename == nullptr) {
    rtlsdr_read_async(Modes.dev, rtlsdrCallback, nullptr,
                      MODES_ASYNC_BUF_NUMBER, MODES_DATA_LEN);
  } else {
    readDataFromFile();
  }
  return nullptr;
}