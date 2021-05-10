//
// Created by Timmy on 4/14/2021.
//

#ifndef DUMP1090_PACKETHANDLING_H
#define DUMP1090_PACKETHANDLING_H
#include <mutex>
#include <vector>
#include <atomic>
#include <semaphore>

struct packet;
struct OpenCL_dat;
OpenCL_dat load_gpu(packet& r_packet);

std::vector<packet> raw_packets;
std::vector<packet> finished_packets;
std::mutex m_finished_packets;

std::mutex m_raw_packets;

bool init_gpu();
void insert_packet(packet& to_store, std::vector<packet>& storage, std::muted& mutx);
void gpu_loader();
#endif //DUMP1090_PACKETHANDLING_H
