//
// Created by Timmy on 4/13/2021.
//
/* Thread1 puts faulty packet into queue
 * thread2 takes faulty packets, feeds it to GPU, without waiting for it to finish. Run this in a loop?
 * Once a packet is finished flipping, calculate crcs and compare.
 * Feed finished packet into queue that is retrieved by thread1 and used.



MAIN THREAD:
    packet = Finished_packets.pop();
    // fix_packet is placed into GPU_LOADER_THREAD now
    Display_Packet();

SDR_THREAD:
    rawPacketsQueue.insert(Get_data());

GPU_LOADER_THREAD:
    for packet in rawPacketsQueue{
        packet = rawPacketsQueue.pop();
        Load_GPU(packet);
    }
    wait(GPU);
    for packet in GPUOutputPacketsQueue:
        if calculate_crc(packet) == GPUOutputPacketsQueue.crc:
            Finished_packets.insert(packet);

 */
#include "../Headers/PacketHandling.h"



bool init_gpu(){
    /*Get device context, devices, create a queue, compile kernel. */

}





packet::packet(std::string &newdata, int64_t sequence, int32_t crc) {
    this->data_size = newdata.size();
    this->data = new char[this->data_size];
    strncpy(this->data, newdata.c_str(), newdata.length());
    this->sequenceNumber = sequence;
    this->crc = crc;
}

packet::~packet() {
    delete[] this->data;
}

packet::packet() {
    this->data = nullptr;
    this->data_size = 0;
    this->sequenceNumber = 0;
    this->crc = 0;
}

packet::packet(packet &other) {
    this->data = new char[other.data_size];
    strncpy(this->data, other.data, other.data_size);
    this->data_size = other.data_size;
    this->sequenceNumber = other.sequenceNumber;
    this->crc = other.crc;
}

packet& packet::operator=(packet const &other) {
    this->data = new char[other.data_size];
    this->data_size = other.data_size;
    strncpy(this->data, other.data, this->data_size);
    this->sequenceNumber = other.sequenceNumber;
    this->crc = other.crc;
    return *this;
}

packet &packet::operator=(packet &&other) noexcept{
    this->data_size = other.data_size;
    this->data = other.data;
    this->sequenceNumber = other.sequenceNumber;
    this->crc = other.crc;

    other.data = nullptr;
    other.data_size = 0;
    return *this;
}
