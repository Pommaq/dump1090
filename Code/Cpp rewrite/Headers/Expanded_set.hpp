//
// Created by timmy on 2021-06-05.
//

#ifndef DUMP1090_EXPANDED_SET_HPP
#define DUMP1090_EXPANDED_SET_HPP

#include <vector>
#include <queue>
#include <shared_mutex>
#include <thread>
#include "../Headers/PacketHandling.hpp"

template<typename T>
class equeue : public std::queue<T> {
public:
    std::shared_lock<std::mutex> lock;

    void insert_packet(packet &to_store);

    packet &&move_packet(int64_t id);

    equeue() = default;

    ~equeue() = default;
};


#endif //DUMP1090_EXPANDED_SET_HPP
