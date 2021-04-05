//
// Created by csather on 4/4/21.
//

#ifndef SLIDING_WINDOW_CONNECTIONCONTROLLER_H
#define SLIDING_WINDOW_CONNECTIONCONTROLLER_H

#include <string>
#include <vector>

#include "Connection.h"
#include "ConnectionInfo.h"

#define KB 1024

using namespace std;

class ConnectionController {
    // Server variables
    unsigned int listeningPort;
    unsigned char maxConnections;

    vector<Connection> openConnectionsBuffer;

    // Opens a new socket which the server will use to accept data from a client
    int openClientSocket();
    int openConnection(string ipAddress, ConnectionInfo connInfo);
public:
    void openConnections(vector<string> ipAddresses, ConnectionInfo connInfo);
    int startServer(string ipAddress, int listeningPort);

};
#endif //SLIDING_WINDOW_CONNECTIONCONTROLLER_H
