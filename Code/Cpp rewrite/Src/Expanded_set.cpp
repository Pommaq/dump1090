//
// Created by timmy on 2021-06-05.
//

#include "../Headers/Expanded_set.hpp"

template<typename T>
void equeue<T>::insert_packet(packet &to_store) {
    this->lock.lock();
    this->insert(to_store);
    this->lock.unlock();
}

template<typename T>
packet&& equeue<T>::move_packet(int64_t id) {
    packet* tmp;
    this->lock.lock();
    auto it = this->find(id);
    if (it != std::queue<T>::end()){
        tmp = std::move(*it);
    }
    this->lock.unlock();

    return std::move(*tmp);
}


