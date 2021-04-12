//
// Created by csather on 4/4/21.
//

#ifndef SLIDING_WINDOW_CONNECTION_H
#define SLIDING_WINDOW_CONNECTION_H

#include <sys/time.h>
#include <vector>
#include <queue>
#include <netdb.h>

#include "Packet.h"
#include "PacketInfo.h"

enum Status {PENDING, OPEN, CLOSED, ERROR};

struct Connection {
    Status status = PENDING;
    struct sockaddr_in srcAddr {0,0,0,0};
    struct sockaddr_in destAddr = {0, 0, 0, 0};
    int sockfd;
    unsigned int sqn;
    unsigned short wSize; // (R|S)WS
    unsigned int pktSizeBytes;
    unsigned int minPktsNeeded;
    unsigned int pktsSent = 0;
    timeval timeoutInterval{0};
    timeval timeConnectionStarted;
    FILE *file; // the file being read or written
//    size_t bytesRead;

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
    vector<PacketInfo> pktBuffer;

    // timeout queue
    queue<PacketInfo*> timeoutQueue;
};


#endif //SLIDING_WINDOW_CONNECTION_H
