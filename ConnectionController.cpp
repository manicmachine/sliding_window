//
// Created by csather on 4/4/21.
//

#include <arpa/inet.h>
#include <string.h>
#include <thread>

#include "ConnectionController.h"
#include "PacketBuilder.h"

using namespace std;

// Open socket, establish a connection, and perform initial handshake
void ConnectionController::handleConnection(Connection &connection) {
    char convertedIP[INET_ADDRSTRLEN];
    unsigned int port;

    if (connection.status == PENDING) {
        // Connect to pending connection
        if (connect(connection.sockfd, (struct sockaddr *) &connection.destAddr, sizeof(connection.destAddr)) < 0) {
            inet_ntop(AF_INET, &connection.destAddr.sin_addr, convertedIP, INET_ADDRSTRLEN);
            unsigned int port = htons(connection.destAddr.sin_port);

            fprintf(stderr, "Failed to connect to %s:%d\nError #: %d\n", convertedIP, port, errno);
            connection.status = ERROR;

            return;
        }

        gettimeofday(&connection.timeConnectionStarted, NULL);
        connection.status = OPEN;
    }

    if (appState->verbose) {
        inet_ntop(AF_INET, &connection.srcAddr.sin_addr, convertedIP, INET_ADDRSTRLEN);
        port = htons(connection.srcAddr.sin_port);
        printf("Source info: Address: %s, Port: %i\n", convertedIP, port);

        inet_ntop(AF_INET, &connection.destAddr.sin_addr, convertedIP, INET_ADDRSTRLEN);
        port = htons(connection.destAddr.sin_port);
        printf("Connection made to %s:%d\n", convertedIP, port);

    }

    handshake(connection);
}

int ConnectionController::startServer() {
    int option = 1;
    sockaddr_in serverAddr = {0,0,0,0};
    int serverSockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (serverSockfd < 0) {
        fprintf(stderr, "Socket creation error\nError #: %d\n", errno);
        return -1;
    }

    if (setsockopt(serverSockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &option, sizeof(option))) {
        fprintf(stderr, "Failed to set socket options\nError #: %d\n", errno);
        return -1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(appState->connectionSettings.port);

    if (bind(serverSockfd, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
        fprintf(stderr, "Failed to bind socket to the port %d\nError #: %d\n", appState->connectionSettings.port, errno);
        return -1;
    }

    if (appState->verbose) {
        printf("Server bound and listening on port %i\n", appState->connectionSettings.port);
    }

    if (listen(serverSockfd, 1) < 0) {
        fprintf(stderr, "Failed to begin listening on port %d\nError #: %d\n", appState->connectionSettings.port, errno);
        return -1;
    }


    do {
        sockaddr_in clientAddr = {0,0,0,0};
        socklen_t clientAddrLen = sizeof(clientAddr);
        Connection connection = createConnection(accept(serverSockfd, (struct sockaddr *) &clientAddr, &clientAddrLen), clientAddr, serverAddr);

        pendingConnections.push(connection);
        processConnections();
    } while (true);

    close(serverSockfd);
}

ConnectionController::ConnectionController(ApplicationState &appState) {
    this->appState = &appState;
}

void ConnectionController::initializeConnections(const string& ipAddress) {
    if (appState->role == CLIENT) {
        for (auto &ipAddress : appState->ipAddresses) {
            pendingConnections.push(createConnection(ipAddress));
        }
    } else {
        pendingConnections.push(createConnection(ipAddress));
    }
}

// Client implementation
Connection ConnectionController::createConnection(const string& ipAddress, bool isPing) {
    Connection connection{};
    connection.sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (connection.sockfd < 0) {
        fprintf(stderr, "Socket creation error\nError #: %d\n", errno);
        connection.status = ERROR;
    }

    connection.destAddr.sin_family = AF_INET;
    connection.destAddr.sin_port = htons(appState->connectionSettings.port);
    inet_pton(AF_INET, ipAddress.c_str(), &connection.destAddr.sin_addr);

    socklen_t len = sizeof(connection.srcAddr);
    getsockname(connection.sockfd, (sockaddr *) &connection.srcAddr, &len);

    connection.sqn = 0;
    connection.wSize = appState->connectionSettings.wSize;
    connection.pktSizeBytes = KB * appState->connectionSettings.pktSize;

    if (!isPing) {
        connection.timeoutInterval = appState->connectionSettings.timeoutInterval;
    } else {
        connection.timeoutInterval.tv_sec = 10;
    }

    // Set socket timeout interval
    if (setsockopt(connection.sockfd, SOL_SOCKET, SO_RCVTIMEO, &connection.timeoutInterval, sizeof(connection.timeoutInterval)) < 0) {
        fprintf(stderr, "Setting socket options failed\nError #: %d\n", errno);
        connection.status = ERROR;
    }

    connection.file = std::fopen(appState->filePath.c_str(), "r");
    connection.minPktsNeeded = (appState->fileStats.st_size / connection.pktSizeBytes) +
            ((appState->fileStats.st_size % connection.pktSizeBytes) > 0 ? 1 : 0 );

    connection.lastRec.lastAckRec = 0;
    connection.lastFrame.lastFrameSent = 0;

    return connection;
}

// Server implementation
Connection ConnectionController::createConnection(int sockfd, sockaddr_in clientAddr, sockaddr_in &serverAddr) {
    Connection connection{};
    connection.sockfd = sockfd;
    connection.status = OPEN;
    gettimeofday(&connection.timeConnectionStarted, NULL);

    if (connection.sockfd < 0) {
        fprintf(stderr, "Socket creation error\nError #: %d\n", errno);
        connection.status = ERROR;
    }

    connection.srcAddr = serverAddr;
    connection.destAddr = clientAddr;

    connection.sqn = 0;
    connection.wSize = appState->connectionSettings.wSize;
    connection.pktSizeBytes = KB * appState->connectionSettings.pktSize;
    connection.timeoutInterval = appState->connectionSettings.timeoutInterval; // TTL

    // Set socket timeout interval
    if (setsockopt(connection.sockfd, SOL_SOCKET, SO_RCVTIMEO, &connection.timeoutInterval, sizeof(connection.timeoutInterval)) < 0) {
        fprintf(stderr, "Setting socket options failed\nError #: %d\n", errno);
        connection.status = ERROR;
    }

    connection.lastRec.lastFrameRec = 0;
    connection.lastFrame.largestAcceptableFrame = 0;

    return connection;
}

string ConnectionController::getLocalAddress() {
    char hostname[HOST_NAME_MAX];
    char *ipAddress;

    gethostname(hostname, sizeof(hostname));
    ipAddress = inet_ntoa(*((struct in_addr*) gethostbyname(hostname)->h_addr_list[0]));

    return ipAddress;
}

void ConnectionController::sendPacket(Connection &connection, PacketInfo &pktInfo) {
    Packet *pkt = pktInfo.pkt;
    char pktBuffer[sizeof(*pkt)];

    memcpy(pktBuffer, pkt, sizeof(*pkt));
    connection.pktsSent++;
    pktInfo.count++;

    if (appState->verbose) {
        if (pkt->header.flags.ack) {
            printf("Ack %i sent\n", pkt->header.sqn);
        } else {
            printf("Packet %i sent\n", pkt->header.sqn);
        }
    }

    if (write(connection.sockfd, pktBuffer, sizeof(pktBuffer)) < 0) {
        fprintf(stderr, "Error writing data to socket\nError #: %d", errno);
        connection.status = ERROR;

        return;
    }

    if (pkt->header.sqn > connection.lastFrame.lastFrameSent) connection.lastFrame.lastFrameSent = pkt->header.sqn;
    pktInfo.timeout = calculateTimeout(connection);
    connection.timeoutQueue.push(&pktInfo);
}

Packet ConnectionController::recPacket(Connection &connection, bool &timeout, bool &badPkt) {
    Packet pkt{};
    char pktBuffer[connection.pktSizeBytes + sizeof(pkt.header)];

    // Listen for response
    if (read(connection.sockfd, pktBuffer, sizeof(pktBuffer)) < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // No packets received within timeout interval
            timeout = true;
        } else {
            fprintf(stderr, "Error reading data from socket\nError #: %d", errno);
            connection.status = ERROR;
        }
    } else {
        memcpy(&pkt, pktBuffer, sizeof(pktBuffer));
        if (appState->verbose) {
            if (appState->role == CLIENT && pkt.header.flags.ack == 1) {
                printf("Ack %i received\n", pkt.header.sqn);
            } else {
                printf("Packet %i received\n", pkt.header.sqn);
            }
        }
    }

    int chksum = pkt.header.chksum;
    pkt.header.chksum = 0;

    if (chksum != PacketBuilder::generateChksum(pkt)) {
        badPkt = true;
        printf("Checksum BAD!\n");
    }

    return pkt;
}

void ConnectionController::transferFile(Connection &connection) {
    while (!connection.pktBuffer.empty() && connection.status == OPEN) {

        // Send packet
        while (connection.pktBuffer.size() <= connection.wSize) {
            PacketInfo pktInfo = connection.pktBuffer[connection.sqn % connection.wSize];
            sendPacket(connection, pktInfo);
        }

        // Read responses


    }

}

void ConnectionController::addToPktBuffer(Connection &connection, Packet pkt) {
    PacketInfo pktInfo{};
    pktInfo.pkt = &pkt;
    pktInfo.count = 0;

    connection.pktBuffer[pkt.header.sqn % connection.wSize] = pktInfo;
}

timeval ConnectionController::calculateTimeout(Connection &connection) {
    timeval timeout{};
    gettimeofday(&timeout, NULL);
    timeout.tv_usec = timeout.tv_usec + connection.timeoutInterval.tv_usec;

    return timeout;
}

void ConnectionController::handshake(Connection &connection, bool isPing) {
    bool timeout = false;
    bool badPkt = false;
    PacketBuilder pktBuilder;
    pktBuilder.setPktSize(connection.pktSizeBytes);
    pktBuilder.setSrcAddr(connection.srcAddr);
    pktBuilder.setDestAddr(connection.destAddr);
    pktBuilder.setWSize(connection.wSize);
    pktBuilder.setPktSize(connection.pktSizeBytes);
    pktBuilder.enableSynBit();

    if (appState->role == CLIENT) {
        // Build SYN packet
        pktBuilder.setSqn(0);
        pktBuilder.setPayload(appState->filePath);
        Packet synPkt = pktBuilder.buildPacket();

        PacketInfo pktInfo{};
        pktInfo.pkt = &synPkt;
        pktInfo.count = 0;

        Packet ackPkt;
        bool acked = false;

        while (pktInfo.count < 3) {
            sendPacket(connection, pktInfo);
            ackPkt = recPacket(connection, timeout, badPkt);

            if (ackPkt.header.flags.ack == 1 && ackPkt.header.flags.syn == 1 && ackPkt.header.sqn == synPkt.header.sqn) acked = true;

            if (!timeout && acked) {
                if (ackPkt.header.wSize != connection.wSize) connection.wSize = ackPkt.header.wSize;
                if (ackPkt.header.pktSize != connection.pktSizeBytes) connection.pktSizeBytes = ackPkt.header.pktSize;
                connection.lastRec.lastAckRec = ackPkt.header.sqn;
            }
        }

        if (!acked) {
            char convertedIP[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &connection.destAddr.sin_addr, convertedIP, INET_ADDRSTRLEN);
            printf("Unable to establish handshake with %s, Exiting\n", convertedIP);
            connection.status = CLOSED;

            return;
        }
    } else {
        // Process initial packet from client
        Packet pkt{}, ackPkt{};

        pkt = recPacket(connection, timeout, badPkt);
        PacketInfo pktInfo{};
        pktInfo.pkt = &ackPkt;
        pktInfo.count = 0;

        while (pkt.header.flags.syn == 1) {
            if (pktInfo.count < 3) {
                if (!timeout && !badPkt) {
                    // Send ACK
                    connection.sqn = pkt.header.sqn;
                    pktBuilder.setSqn(connection.sqn);
                    pktBuilder.enableAckBit();
                    ackPkt = pktBuilder.buildPacket();

                    sendPacket(connection, pktInfo);
                }
            } else {
                char convertedIP[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &connection.destAddr.sin_addr, convertedIP, INET_ADDRSTRLEN);
                printf("Unable to establish handshake with %s, Exiting\n", convertedIP);
                connection.status = CLOSED;

                return;
            }

            // Wait for response
            pkt = recPacket(connection, timeout, badPkt);
        }

        // First data packet received
        pktInfo.pkt = &pkt;
        pktInfo.count = 0;
        connection.sqn = pkt.header.sqn;
        connection.lastRec.lastFrameRec = pkt.header.sqn;
        connection.lastFrame.largestAcceptableFrame = connection.lastRec.lastFrameRec + connection.wSize;

        connection.pktBuffer[connection.sqn % connection.wSize] = pktInfo;
    }

    if (!isPing) {
        // Begin communications
        transferFile(connection);
    } else {
        return;
    }
}

timeval ConnectionController::generatePingBasedTimeout() {
    timeval tv{};
    time_t avgTimeout = 0; // ms
    time_t start;
    time_t end;

    for (int i = 0; i < 3; i++) {
        gettimeofday(&tv, NULL);
        start = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);

        Connection connection = createConnection(appState->ipAddresses[0]);
        gettimeofday(&tv, NULL);
        end = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);

        avgTimeout = avgTimeout + (end - start);
    }

    avgTimeout = avgTimeout / 3;
    avgTimeout = avgTimeout * appState->connectionSettings.timeoutScale;

    tv.tv_usec = avgTimeout * 1000;

    return tv;
}

void ConnectionController::processConnections() {
    while(!pendingConnections.empty()) {
        Connection connection = pendingConnections.front();
        pendingConnections.pop();

        handleConnection(connection);
    }
}

//void ConnectionController::setupWorkers(int numWorkers) {
//    for (int i = 0; i < numWorkers; i++) {
//        std::thread worker(handleConnections);
//        worker.detach();
//        connectionWorkers.emplace_back(worker);
//    }
//}


