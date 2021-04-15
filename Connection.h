//
// Created by csather on 4/4/21.
//

#ifndef SLIDING_WINDOW_CONNECTION_H
#define SLIDING_WINDOW_CONNECTION_H

#include <chrono>
#include <sys/time.h>
#include <vector>
#include <queue>
#include <netdb.h>

#include "Packet.h"
#include "PacketInfo.h"

enum Status {PENDING, OPEN, CLOSED, ERROR, COMPLETE};

struct Connection {
    Status status = PENDING;
    struct sockaddr_in srcAddr {0,0,0,0};
    struct sockaddr_in destAddr = {0, 0, 0, 0};
    int sockfd;
    unsigned int sqn;
    unsigned int sqnBits;
    unsigned short wSize; // (R|S)WS
    unsigned int pktSizeBytes;
    unsigned int pktsSent = 0;
    unsigned int resentPkts = 0;
    unsigned int minPktsNeeded = 1;
    chrono::microseconds timeoutInterval;
    chrono::time_point<chrono::system_clock> timeConnectionStarted;
    string filename;
    FILE *file; // the file being read or written
    size_t bytesRead = 0;

    union lastRec {
        unsigned int lastAckRec = 0; // LAR
        unsigned int lastFrameRec; // LFR
    } lastRec;

    union lastFrame {
        unsigned int largestAcceptableFrame = 0; // LAF
        unsigned int lastFrameSent; // LFS
    } lastFrame;

    // packet buffer
    // Packets inserted at the index (sqn % wSize) with the next in-order packet being (sqn % wSize) + 1
    vector<PacketInfo> pktBuffer;

    // timeout queue
    queue<PacketInfo*> timeoutQueue;
};


#endif //SLIDING_WINDOW_CONNECTION_H
