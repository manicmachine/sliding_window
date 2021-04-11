//
// Created by csather on 4/4/21.
//

#ifndef SLIDING_WINDOW_CONNECTION_H
#define SLIDING_WINDOW_CONNECTION_H

#include <vector>
#include <queue>
#include <netdb.h>

#include "Packet.h"
#include "PacketInfo.h"

struct Connection {
    struct sockaddr_in sockaddr = {0,0,0,0};
    int sockfd;
    unsigned short wSize; // (R|S)WS - need separate wsize to track negotiated wSizes as they may differ from what the user specified
    unsigned int pktSizeBytes;
    unsigned int minPktsNeeded;
    unsigned int pktsSent;
    time_t timeConnectionStarted;
    FILE *file; // the file being read or written

    union lastRec {
        unsigned int lastAckRec; // LAR
        unsigned int lastFrameRec; // LFR
    } lastRec;

    union lastFrame {
        unsigned int largestAcceptableFrame; // LAF
        unsigned int lastFrameSent; // LFS
    } lastFrame;


    // packet buffer
    // Packets inserted at the index (sqn % wSize) with the next in-order packet being (sqn % wSize) + 1
    vector<Packet> pktBuffer;

    // timeout queue
    queue<PacketInfo> timeoutQueue;
};


#endif //SLIDING_WINDOW_CONNECTION_H
