//
// Created by csather on 3/28/21.
//

#ifndef SLIDING_WINDOW_PACKETINFO_H
#define SLIDING_WINDOW_PACKETINFO_H


struct PacketInfo {
    struct Packet *pkt;
    time_t timeout;
    unsigned char count; // tracks the number of times this packet was sent/acked
}


#endif //SLIDING_WINDOW_PACKETINFO_H