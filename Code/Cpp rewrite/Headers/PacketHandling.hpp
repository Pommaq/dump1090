//
// Created by Timmy on 4/14/2021.
//
#pragma once
#include <mutex>
#include <vector>
#include <atomic>
#include <string>
#include <cstring>
#include "data_reader.hpp"

struct packet {
    packet(std::string &newdata, int64_t sequence, int32_t crc = 0);

    packet(packet &other);

    packet &operator=(packet const &other);

    packet &operator=(packet &&other) noexcept;

    packet();

    ~packet();

    char *data;
    unsigned long int data_size;
    int64_t sequenceNumber;
    int32_t crc;
};


