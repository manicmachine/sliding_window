//
// Created by csather on 3/21/21.
//

#ifndef SLIDING_WINDOW_PACKETBUILDER_H
#define SLIDING_WINDOW_PACKETBUILDER_H

#include <algorithm>
#include <array>
#include <cstdint>
#include <numeric>
#include <string.h>
#include <netdb.h>

#include "Packet.h"
#include "boost/crc.hpp"

using namespace std;

class PacketBuilder {
    unsigned int sqn = 0;
    struct sockaddr_in srcAddr {0,0,0,0};
    struct sockaddr_in destAddr {0,0,0,0};
    unsigned short wSize = 0;
    unsigned char sqnbits= 0;
    int pktSize = 0;
    bool ack = false;
    bool syn = false;
    bool fin = false;
    bool ping = false;
    char *payload = NULL;

    void initPayload();

public:
    static int generateChksum(Packet pkt);

    void setPktSize(unsigned int pktSize);

    void setSqn(unsigned int sqn);

    void setSrcAddr(struct sockaddr_in srcAddr);

    void setDestAddr(struct sockaddr_in destAddr);

    void setWSize(unsigned short wSize);

    void setSqnBits(unsigned char bits);

    void setPayload(const char *buffer, int buffLen = 0);

    void enableAckBit();

    void enableSynBit();

    void enableFinBit();

    void enablePingBit();

    void resetFlags();

    struct Packet buildPacket();

};

#endif //SLIDING_WINDOW_PACKETBUILDER_H
