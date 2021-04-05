//
// Created by csather on 3/27/21.
//

#ifndef SLIDING_WINDOW_APPLICATIONSTATE_H
#define SLIDING_WINDOW_APPLICATIONSTATE_H

#include <sys/stat.h>
#include <vector>

#include "ConnectionInfo.h"

using namespace std;

enum Role {
    NO_ROLE, CLIENT, SERVER
};

struct ApplicationState {
    bool verbose = false;
    Role role = NO_ROLE;
    struct ConnectionInfo connInfo{};

    vector<string> ipAddresses;

    string filePath;
    struct stat fileStats{};
    FILE *file;
    size_t bytesRead = 0;
};

#endif //SLIDING_WINDOW_APPLICATIONSTATE_H
