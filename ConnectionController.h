//
// Created by csather on 4/4/21.
//

#ifndef SLIDING_WINDOW_CONNECTIONCONTROLLER_H
#define SLIDING_WINDOW_CONNECTIONCONTROLLER_H

#include <string>
#include <thread>
#include <vector>

#include "ApplicationState.h"
#include "Connection.h"
#include "ConnectionSettings.h"

#define KB 1024

using namespace std;

class ConnectionController {
    size_t bytesRead;
    vector<thread> connectionWorkers;
    queue<Connection> pendingConnections; // Store connections yet to be handled by a thread

public:
    static void handleConnections();
    void setupWorkers(int numWorkers);
    static int openConnection(string ipAddress, ConnectionSettings connInfo, string fileName);
    void openConnections(vector<string> ipAddresses, ConnectionSettings connectionSettings);
    static int processConnection();
    int startServer(vector<string> ipAddresses, ConnectionSettings connectionSettings);
    void getConnection();

};
#endif //SLIDING_WINDOW_CONNECTIONCONTROLLER_H
