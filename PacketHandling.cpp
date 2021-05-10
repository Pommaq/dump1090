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
#include <mutex>
#include <vector>
#include <atomic>
#include <semaphore>
#include "PacketHandling.h"



bool init_gpu(){
    /*Get device context, devices, create a queue, compile kernel. */

}
void insert_packet(packet& to_store, std::vector<packet>& storage, std::muted& mutx){
    while(std::mutex::try_lock(mutx));
    storage.push_back(to_store);
    std::mutex::unlock(mutx);
}


void gpu_loader(){
    static std::vector<packet> fixed_packets;
    OpenCL_dat gpu_data = init_gpu();
    if (gpu_data.error == ERROR){
        modes.exit = true; // An error occured. Signal other threads to exit
    }
    while (!modes.exit){
        while(std::mutex::try_lock(m_raw_packets)); // spinlock
        if (raw_packets.size() > 0){
            for (auto r_packet: raw_packets){
                raw_packets.pop_back();
                if (packet_needs_fixing(r_packet)){
                    load_gpu(r_packet);
                }
                else{
                    insert_packet(r_packet, finished_packets, m_finished_packets);
                }
            }
            raw_packets.clear();
            std::mutex::unlock(m_raw_packets);
            clFinish(g_OpenCL_Dat.queue); // TODO: Replace with C++ equivalent

            fixed_packets = get_returns(gpu); // TODO: Implement
            for (packet& f_packet: fixed_packets){
                unsigned int crc2 = calculate_crc(f_packet);
                if (crc1 == crc2){
                    insert_packet(f_packet, finished_packets, m_finished_packets);
                }
            }
            fixed_packets.clear();

        }
        // else exit
    }
}