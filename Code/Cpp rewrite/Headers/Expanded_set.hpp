//
// Created by timmy on 2021-06-05.
//

#ifndef DUMP1090_EXPANDED_SET_HPP
#define DUMP1090_EXPANDED_SET_HPP

#include <vector>
#include <queue>
#include <mutex>
#include <thread>
#include "../Headers/PacketHandling.hpp"
#include "modesMessage.h"

template<typename T>
class equeue : protected std::queue<T> {
private:
    std::mutex _lock;
public:


    void insert_packet(T &to_store);

    T &&move_packet(int64_t id);

    equeue() = default;

    ~equeue() = default;
};

template<typename T>
void equeue<T>::insert_packet(T &to_store) {
    std::lock_guard lk(this->_lock);
    this->emplace(to_store);
}

template<typename T>
T&& equeue<T>::move_packet(int64_t id) {
    T* tmp;
    std::lock_guard lk(this->_lock);

    auto it = this->find(id);
    if (it != std::queue<T>::end()){
        tmp = std::move(*it);
    }

    return std::move(*tmp);
}

#endif //DUMP1090_EXPANDED_SET_HPP
