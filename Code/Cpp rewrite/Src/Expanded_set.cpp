//
// Created by timmy on 2021-06-05.
//

#include "../Headers/Expanded_set.h"

template<typename T>
void eset<T>::insert_packet(packet &to_store) {
    this->lock.lock();
    this->insert(to_store);
    this->lock.unlock();
}

template<typename T>
packet&& eset<T>::move_packet(int64_t id) {
    packet* tmp;
    this->lock.lock();
    auto it = this->find(id);
    if (it != std::set<T>::end()){
        tmp = std::move(*it);
    }
    this->lock.unlock();

    return std::move(*tmp);
}

