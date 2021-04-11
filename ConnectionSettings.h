//
// Created by csather on 4/4/21.
//

#ifndef SLIDING_WINDOW_CONNECTIONSETTINGS_H
#define SLIDING_WINDOW_CONNECTIONSETTINGS_H

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
    time_t timeoutInterval = 0;
    const float timeoutScale = 0.9;
    unsigned short wSize = 0;
    unsigned int sqnRange = 0;
    float damageProb = -1; // Negative value signals user hasn't confirmed value yet
    vector<int> damagedPackets;
};


#endif //SLIDING_WINDOW_CONNECTIONSETTINGS_H
