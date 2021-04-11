//
// Created by csather on 4/4/21.
//

#include <thread>

#include "ConnectionController.h"

using namespace std;

// Open socket and connect with server, returning the socket filePath descriptor. If return < 0, then an error occurred
int ConnectionController::openConnection(string ipAddress, ConnectionSettings connInfo) {
    struct Connection connection{};

    // TODO: Finish implementing
//    connection.sockaddr

//    struct sockaddr_in serverAddr = {0, 0, 0, 0};
//    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
//
//    if (sockfd < 0) {
//        fprintf(stderr, "Socket creation error\nError #: %d\n", errno);
//        return -1;
//    }
//
//    serverAddr.sin_family = AF_INET;
//    serverAddr.sin_port = htons(port);
//
//    //
//    if (inet_pton(AF_INET, ipAddress.c_str(), &serverAddr.sin_addr) < 1) {
//        fprintf(stderr, "Invalid IP address provided\nError #: %d\n", errno);
//        return -1;
//    }
//
//    if (connect(sockfd, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
//        fprintf(stderr, "Failed to connect to %s:%d\nError #: %d\n", ipAddress.c_str(), port, errno);
//        return -1;
//    }
//
//    if (verbose) {
//        printf("Connection made to %s:%d\n", ipAddress.c_str(), port);
//    }
//
//    return sockfd;
}

void ConnectionController::openConnections(vector<string> ipAddresses, ConnectionSettings connectionSettings) {
    for (auto &ipAddress : ipAddresses) {
        openConnection(ipAddress, connectionSettings);
    }
}

void ConnectionController::setupWorkers(int numWorkers) {
    for (int i = 0; i < numWorkers; i++) {
        std::thread worker(handleConnections);
        worker.detach();
        connectionWorkers.emplace_back(worker);
    }
}


