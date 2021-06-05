//
// Created by timmy on 2021-06-05.
//

#ifndef DUMP1090_EXPANDED_SET_H
#define DUMP1090_EXPANDED_SET_H
#include <vector>
#include <set>
#include <shared_mutex>
#include "../Headers/PacketHandling.h"

template <typename T>
class eset: public std::set<int64_t,T>{
public:
    std::shared_lock<std::mutex> lock;
    void insert_packet(packet& to_store);
    packet&& move_packet(int64_t id);
    eset() = default;
    ~eset() = default;
};


#endif //DUMP1090_EXPANDED_SET_H
