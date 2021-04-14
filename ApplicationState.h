//
// Created by csather on 3/27/21.
//

#ifndef SLIDING_WINDOW_APPLICATIONSTATE_H
#define SLIDING_WINDOW_APPLICATIONSTATE_H

#include <sys/stat.h>
#include <vector>

#include "ConnectionSettings.h"

using namespace std;

enum Role {
    NO_ROLE, CLIENT, SERVER
};

struct ApplicationState {
    bool verbose = false;
    Role role = NO_ROLE;
    struct ConnectionSettings connectionSettings{};

    /* Server - List of IP Addresses from which to accept connections
     * Client - List of IP Address to creation connections to
     */
    vector<string> ipAddresses;

    string filePath;
    string fileName;
    struct stat fileStats{};
};

#endif //SLIDING_WINDOW_APPLICATIONSTATE_H
