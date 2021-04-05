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
#include "boost/crc.hpp"

using namespace std;

class PacketBuilder {
    unsigned int sqn;
    struct sockaddr_in srcAddr;
    struct sockaddr_in destAddr;
    unsigned short wSize;
    bool brokenChksum = false;
    boost::crc_32_type chksum;
    bool ack = false;
    bool syn = false;
    bool fin = false;
    char *payload;

    void generateChksum(Packet pkt, bool broken = false);
public:
    void setSqn(unsigned int sqn);

    void setSrcAddr(struct sockaddr_in srcAddr);

    void setDestAddr(struct sockaddr_in destAddr);

    void setWSize(unsigned short wSize);

    void setPayload(char *buffer);

    void enableBrokenChksum();

    void enableAckBit();

    void enableSynBit();

    void enableFinBit();

    struct Packet buildPacket();
};

#endif //SLIDING_WINDOW_PACKETBUILDER_H
