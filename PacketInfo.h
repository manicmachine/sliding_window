//
// Created by csather on 3/28/21.
//

#ifndef SLIDING_WINDOW_PACKETINFO_H
#define SLIDING_WINDOW_PACKETINFO_H


struct PacketInfo {
    struct Packet *pkt;
    chrono::time_point<chrono::system_clock> timeout{};
    unsigned char count = 0; // tracks the number of times this packet was sent/acked
    bool acked = false;
};


#endif //SLIDING_WINDOW_PACKETINFO_H