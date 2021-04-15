//
// Created by csather on 4/4/21.
//

#ifndef SLIDING_WINDOW_CONNECTIONSETTINGS_H
#define SLIDING_WINDOW_CONNECTIONSETTINGS_H

#include <sys/time.h>
#include <chrono>
#include <vector>

using namespace std;

enum Protocol {
    NO_PROTO, SR, GBN
};


struct ConnectionSettings {
    Protocol protocol = NO_PROTO;
    unsigned char maxConnections = 1;
    unsigned int pktSize = 0;
    unsigned int port = 0;
    chrono::microseconds timeoutInterval = chrono::microseconds (0); // Used to calculate TTL for server
    const float timeoutScale = 0.9;
    unsigned short wSize = 0;
    unsigned int sqnRange = 0;
    unsigned char sqnBits = 0;
    float damageProb = -1; // Negative value signals user hasn't confirmed value yet
    float lostProb = -1;
    vector<int> damagedPackets{};
    vector<int> lostPackets{};
};


#endif //SLIDING_WINDOW_CONNECTIONSETTINGS_H
