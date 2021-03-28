//
// Created by csather on 3/27/21.
//

#ifndef SLIDING_WINDOW_APPLICATIONSTATE_H
#define SLIDING_WINDOW_APPLICATIONSTATE_H

using namespace std;

enum Role {
    NO_ROLE, CLIENT, SERVER
};
enum Protocol {
    NO_PROTO, SR, GBN
};

struct ApplicationState {
        Role role = NO_ROLE;
        Protocol protocol = NO_PROTO;
        int sockfd;
        unsigned int pktSize = 0;
        string ipAddress = "";
        string localIPAddress = "";
        unsigned int port = 0;
        string file = "";
        time_t timeoutInterval = 0;
        FILE *pFile;
        size_t bytesRead = 0;
        bool verbose = false;
        unsigned short wSize = 0;
        unsigned int sqnRange = 0;

        // Situational errors
        // - Randomly generated
        // - User specified
        // - - Drop packages
        // - - Lose acks
};

#endif //SLIDING_WINDOW_APPLICATIONSTATE_H
