//
// Created by timmy on 2021-06-05.
//

#ifndef DUMP1090_DATA_READER_HPP
#define DUMP1090_DATA_READER_HPP

#include "rtl-sdr.h"
#include "anet.h"
#include "Modes.hpp"
#include <thread>
#include <mutex>
#include <condition_variable>


void modesInitRTLSDR(void);

void rtlsdrCallback(unsigned char *buf, uint32_t len, void *ctx);

void readDataFromFile(void);

void *readerThreadEntryPoint(void *arg);


#endif // DUMP1090_DATA_READER_HPP
