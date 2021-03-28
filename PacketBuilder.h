//
// Created by csather on 3/21/21.
//

#ifndef SLIDING_WINDOW_PACKETBUILDER_H
#define SLIDING_WINDOW_PACKETBUILDER_H

#include <algorithm>
#include <array>
#include <cstdint>
#include <numeric>

#include <netdb.h>

#include "Packet.h"

using namespace std;

class PacketBuilder {
    unsigned int sqn;
    struct sockaddr_in srcAddr;
    struct sockaddr_in destAddr;
    unsigned short wSize;
    uint_fast32_t chksum;
    bool ack = false;
    bool syn = false;
    bool fin = false;
    char *payload;

    template<typename InputIterator>
    uint_fast32_t generateChksum(InputIterator first, InputIterator last);

    array<std::uint_fast32_t, 256> generateCrcLookupTable();

public:
    void setSqn(unsigned int sqn);

    void setSrcAddr(struct sockaddr_in srcAddr);

    void setDestAddr(struct sockaddr_in destAddr);

    void setWSize(unsigned short wSize);

    void setChksum(uint_fast32_t chksum);

    void setPayload(char *buffer);

    void enableAckBit();

    void enableSynBit();

    void enableFinBit();

    struct Packet buildPacket();
};

#endif //SLIDING_WINDOW_PACKETBUILDER_H
