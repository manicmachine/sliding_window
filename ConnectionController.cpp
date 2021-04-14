//
// Created by csather on 4/4/21.
//

#include <arpa/inet.h>
#include <string.h>
#include <thread>

#include "ConnectionController.h"
#include "PacketBuilder.h"

using namespace std;

ConnectionController::ConnectionController(ApplicationState &appState) {
    srand(chrono::system_clock::now().time_since_epoch().count()); // seed random number generator with the time
    this->appState = &appState;
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
        int clientfd = accept(serverSockfd, (struct sockaddr *) &clientAddr, &clientAddrLen);
        Connection connection = createConnection(clientfd, clientAddr, serverAddr);

        pendingConnections.push(connection);
        processConnections();
    } while (true);

    close(serverSockfd);
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

void ConnectionController::processConnections() {
    while(!pendingConnections.empty()) {
        Connection connection = pendingConnections.front();
        pendingConnections.pop();

        handleConnection(connection);
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
    inet_pton(AF_INET, getLocalAddress().c_str(), &(connection.srcAddr.sin_addr));

    connection.sqn = 0;
    connection.wSize = appState->connectionSettings.wSize;
    connection.sqnBits = appState->connectionSettings.sqnBits;
    connection.pktSizeBytes = KB * appState->connectionSettings.pktSize;

    if (!isPing) {
        connection.timeoutInterval = appState->connectionSettings.timeoutInterval;
    } else {
        connection.timeoutInterval = chrono::seconds(5);
    }

    // Set socket timeout interval
    timeval timeout = toTimeval(connection.timeoutInterval);
    if (setsockopt(connection.sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        fprintf(stderr, "Setting socket options failed\nError #: %d\n", errno);
        connection.status = ERROR;
    }

    connection.file = std::fopen(appState->filePath.c_str(), "r");
    connection.minPktsNeeded = (appState->fileStats.st_size / connection.pktSizeBytes) +
                               ((appState->fileStats.st_size % connection.pktSizeBytes) >= 0 ? 1 : 0 );

    connection.lastRec.lastAckRec = 0;
    connection.lastFrame.lastFrameSent = 0;

    return connection;
}

// Server implementation
Connection ConnectionController::createConnection(int sockfd, sockaddr_in clientAddr, sockaddr_in &serverAddr) {
    Connection connection{};
    connection.sockfd = sockfd;
    connection.status = OPEN;
    connection.timeConnectionStarted = chrono::system_clock::now();

    if (connection.sockfd < 0) {
        fprintf(stderr, "Socket creation error\nError #: %d\n", errno);
        connection.status = ERROR;
    }

    connection.srcAddr = serverAddr;
    connection.destAddr = clientAddr;

    connection.sqn = 0;
    connection.wSize = appState->connectionSettings.wSize;
    connection.pktSizeBytes = KB * appState->connectionSettings.pktSize;
    connection.sqnBits = appState->connectionSettings.sqnBits;
    connection.timeoutInterval = appState->connectionSettings.timeoutInterval; // TTL

    // Set socket timeout interval
    timeval timeout = toTimeval(connection.timeoutInterval);
    if (setsockopt(connection.sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        fprintf(stderr, "Setting socket options failed\nError #: %d\n", errno);
        connection.status = ERROR;
    }

    connection.lastRec.lastFrameRec = 0;
    connection.lastFrame.largestAcceptableFrame = 0;

    return connection;
}

// Open socket, establish a connection, and perform initial handshake
void ConnectionController::handleConnection(Connection &connection, bool isPing) {
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

        connection.timeConnectionStarted = chrono::system_clock::now();
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

    handshake(connection, isPing);
}

void ConnectionController::sendPacket(Connection &connection, PacketInfo &pktInfo) {
    bool lost = false;
    Packet *pkt = pktInfo.pkt;
    connection.pktsSent++;
    pktInfo.count++;

    if (appState->connectionSettings.damagedPackets.front() == pkt->header.sqn) {
        pkt->header.chksum = ~pkt->header.chksum;
        appState->connectionSettings.damagedPackets.erase(appState->connectionSettings.damagedPackets.begin());
    } else if (packetBadLuck(appState->connectionSettings.damageProb)) {
        pkt->header.chksum = ~pkt->header.chksum;
    } else if (appState->connectionSettings.lostPackets.front() == pkt->header.sqn) {
        lost = true;
        appState->connectionSettings.lostPackets.erase(appState->connectionSettings.lostPackets.begin());
    } else if (packetBadLuck(appState->connectionSettings.lostProb)) {
        lost = true;
    }

    if (!lost) {
        // Send header
        if (write(connection.sockfd, (Packet::Header *) &(pkt->header), sizeof(Packet::Header)) < 0) {
            fprintf(stderr, "Error writing header to socket\nError #: %d\n", errno);
            connection.status = ERROR;

            return;
        }

        if (pkt->header.pktSize != 0 && (pkt->header.flags.syn != 1 && pkt->header.flags.ack != 1)) {
            // Send payload
            if (write(connection.sockfd, (char *) pkt->payload, pkt->header.pktSize) < 0) {
                fprintf(stderr, "Error writing payload to socket\nError #: %d\n", errno);
                connection.status = ERROR;

                return;
            }
        }
    }


    if (appState->verbose) {
        if (pkt->header.flags.ack == 1) {
            printf("Ack %u sent\n", pkt->header.sqn);
        } else {
            printf("Packet %u sent\n", pkt->header.sqn);
        }

        if (lost) {
            printf("Packet LOST in transmission\n");
        }
    }

    if (pkt->header.sqn > connection.lastFrame.lastFrameSent) connection.lastFrame.lastFrameSent = pkt->header.sqn;
    pktInfo.timeout = chrono::system_clock::now() + connection.timeoutInterval;
    connection.timeoutQueue.push(&pktInfo);
}

Packet ConnectionController::recPacket(Connection &connection, bool &timeout, bool &badPkt) {
    Packet pkt{};

    // Listen for response
    // Read header
    if (read(connection.sockfd, (Packet::Header *) &(pkt.header), sizeof(Packet::Header)) < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // No packets received within timeout interval
            timeout = true;
            printf("Timed out waiting for packet\n");
        } else {
            fprintf(stderr, "Error reading header from socket\nError #: %d\n", errno);
            connection.status = ERROR;
        }
    } else {
        // Don't initialize packet payload if pktsize == 0 (no payload) or it's a SYN/ACK packet (pktsize in negotiation)
        if (pkt.header.pktSize != 0 && (pkt.header.flags.syn != 1 && pkt.header.flags.ack != 1)) {
            pkt.initPayload();

            // Read payload
            if (read(connection.sockfd, (char *) pkt.payload, pkt.header.pktSize) < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // No packets received within timeout interval
                    timeout = true;
                    printf("Timed out waiting for payload\n");
                } else {
                    fprintf(stderr, "Error reading payload from socket\nError #: %d\n", errno);
                    connection.status = ERROR;
                }
            }
        }

        if (appState->verbose) {
            if (appState->role == CLIENT && pkt.header.flags.ack == 1 && !timeout) {
                printf("Ack %u received\n", pkt.header.sqn);
            } else {
                printf("Packet %u received\n", pkt.header.sqn);
            }
        }
    }

    if (!timeout) {
        // Check if damaged
        int chksum = pkt.header.chksum;
        pkt.header.chksum = 0;

        if (chksum != PacketBuilder::generateChksum(pkt)) {
            badPkt = true;
            printf("Checksum BAD!\n");
        }
    }

    // TODO: Do we need to do something about timeouts here?
    return pkt;
}

// Implements full round trip communication between client/server, returning the ack'd packet
Packet ConnectionController::sendAndRec(Connection &connection, PacketInfo &pktInfo, bool &timeout, bool &badPkt) {
    Packet pkt;
    bool acked = false;

    // Resend packet up to RETRY times or until ack'd
    while (pktInfo.count < RETRY && pkt.header.flags.ack != 1) {
        sendPacket(connection, pktInfo);
        pkt = recPacket(connection, timeout, badPkt);

        if (pkt.header.flags.ack == 1 && pkt.header.sqn == pktInfo.pkt->header.sqn) {
            acked = true;
            break;
        }

        if (badPkt) {
            if (pktInfo.count < RETRY && pkt.header.flags.ack != 1) badPkt = false;
            continue;
        }
    }

    // We've exceeded retry limit but haven't received an ACK so close connection
    if (!acked || badPkt || timeout || (pktInfo.pkt->header.flags.syn == 1 && pkt.header.flags.syn != 1)) connection.status = CLOSED;

    return pkt;
}

// TODO: Implement me
// Receives a packet from the client and acks it, returning a valid client data packet
Packet ConnectionController::recAndAck(Connection &connection, bool &timeout, bool &badPkt) {
    bool validPkt = false;
    Packet pkt, ackPkt{};
    PacketInfo pktInfo{};
    pktInfo.pkt = &ackPkt;

    PacketBuilder pktBuilder;
    pktBuilder.setSrcAddr(connection.srcAddr);
    pktBuilder.setDestAddr(connection.destAddr);
    pktBuilder.setWSize(connection.wSize);
    pktBuilder.setSqnBits(connection.sqnBits);

     do {
        // Wait for next packet
        pkt = recPacket(connection, timeout, badPkt);

        // If we've timed out, then the connection exceeded it's TTL so we need to close the connection
        if (timeout) break;

        // Packet damaged or right of window
        if (badPkt || pkt.header.sqn > (connection.lastRec.lastFrameRec + connection.wSize)) {
            // Invalid packet; discard
            badPkt = false;
            continue;
        }

        // Check if within window and a valid data packet for return; if left of window, just send ACK
        if ((pkt.header.sqn > connection.lastRec.lastFrameRec) &&
                (pkt.header.sqn <= (connection.lastRec.lastFrameRec + connection.wSize)) &&
                (pkt.header.flags.ping != 1 && pkt.header.flags.syn != 1)) {
           validPkt = true;
        }

        // Send ACK
        pktBuilder.setSqn(pkt.header.sqn);
        pktBuilder.enableAckBit();

        if (pkt.header.flags.ping != 1) {
            if (pkt.header.flags.syn == 1) {
                pktBuilder.enableSynBit();
                pktBuilder.setPktSize(connection.pktSizeBytes);

                connection.filename = pkt.payload;
            } else if (pkt.header.flags.fin == 1) {
                pktBuilder.enableFinBit();
            }
        }

        ackPkt = pktBuilder.buildPacket();
        sendPacket(connection, pktInfo);

        // No need to return PING or SYN packets, just ACK and continue
        if (pkt.header.flags.ping == 1 || pkt.header.flags.syn == 1) continue;

         // Next packet in sequence
         if (pkt.header.sqn == (connection.lastRec.lastFrameRec + 1)) connection.lastRec.lastFrameRec++;
    } while (!validPkt);

    // Connection exceeded TTL
    if (timeout) connection.status = CLOSED;

    // Return valid pkt
    return pkt;
}

void ConnectionController::transferFile(Connection &connection) {
    // Client
    // Read file and send chunks along to server

    // Server
}

//
void ConnectionController::addToPktBuffer(Connection &connection, Packet pkt) {
    PacketInfo pktInfo{};
    pktInfo.pkt = &pkt;

    connection.pktBuffer[pkt.header.sqn % connection.wSize] = pktInfo;
}

void ConnectionController::handshake(Connection &connection, bool isPing) {
    bool timeout = false;
    bool badPkt = false;

    if (appState->role == CLIENT) {
        // Client
        // Create SYN packet and wait for SYN/ACK
        PacketBuilder pktBuilder;
        pktBuilder.setSrcAddr(connection.srcAddr);
        pktBuilder.setDestAddr(connection.destAddr);
        pktBuilder.setSqn(connection.sqn);
        pktBuilder.setSqnBits(connection.sqnBits);
        pktBuilder.setWSize(connection.wSize);
        pktBuilder.setPktSize(connection.pktSizeBytes);
        pktBuilder.enableSynBit();

        if (isPing) {
            pktBuilder.enablePingBit();
        } else {
            pktBuilder.setPayload(appState->filePath);
        }

        Packet synPkt = pktBuilder.buildPacket(), ackPkt{};

        PacketInfo pktInfo{};
        pktInfo.pkt = &synPkt;

        ackPkt = sendAndRec(connection,pktInfo, timeout, badPkt);
        if (connection.status == CLOSED) {
            char convertedIP[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &connection.destAddr.sin_addr, convertedIP, INET_ADDRSTRLEN);
            printf("Unable to establish handshake with %s, Closing connection\n", convertedIP);
            return;
        }

        // Adjust parameters to align with server
        if (ackPkt.header.wSize != connection.wSize) connection.wSize = ackPkt.header.wSize;
        if (ackPkt.header.pktSize != connection.pktSizeBytes) connection.pktSizeBytes = ackPkt.header.pktSize;
        if (ackPkt.header.sqnBits != connection.sqnBits) connection.sqnBits = ackPkt.header.sqnBits;
        connection.lastRec.lastAckRec = ackPkt.header.sqn;
    } else {
        // Server
        // Wait for SYN packet and respond with SYN/ACK
        // recAndAck takes care of responding to ping packets, so returned packet is first data packet
        Packet pkt = recAndAck(connection, timeout, badPkt);
        connection.file = std::fopen(appState->filePath.append("/").append(connection.filename).c_str(), "w+");

        // Create file with first data packet received before moving to transfer phase
        fwrite(pkt.payload, sizeof(char), pkt.header.pktSize, connection.file);
    }

    // Both
    // If not Ping, transition to transfer phase
    if (!isPing && connection.status == OPEN) transferFile(connection);
}

chrono::microseconds ConnectionController::generatePingBasedTimeout() {
    chrono::time_point<chrono::system_clock> start;
    chrono::microseconds avgTimeout = chrono::microseconds (0); // ms
    int scalar = appState->connectionSettings.timeoutScale * 100; // Issues happen when multiplying chrono units with floats, so make int first then divide by 10

    for (int i = 0; i < 5; i++) {
        start = chrono::system_clock::now();
        Connection connection = createConnection(appState->ipAddresses[0], true);
        handleConnection(connection, true);
        avgTimeout = avgTimeout + chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now() - start);
    }

    avgTimeout = avgTimeout / 5;
    avgTimeout = chrono::duration_cast<chrono::microseconds>((avgTimeout * scalar) / 100);

    return avgTimeout;
}

string ConnectionController::getLocalAddress() {
    char hostname[HOST_NAME_MAX];
    char *ipAddress;

    gethostname(hostname, sizeof(hostname));
    ipAddress = inet_ntoa(*((struct in_addr*) gethostbyname(hostname)->h_addr_list[0]));

    return ipAddress;
}

timeval ConnectionController::toTimeval(chrono::microseconds value) {
    chrono::seconds const seconds = chrono::duration_cast<chrono::seconds>(value);
    timeval tv{};

    tv.tv_sec = seconds.count();
    tv.tv_usec = chrono::duration_cast<chrono::microseconds>(value - seconds).count();

    return tv;
}
// Determines if something should happen to a packet due to a given probability

bool ConnectionController::packetBadLuck(float prob) {
    return (rand() % 100) < ((int) (prob * 100));
}

//void ConnectionController::setupWorkers(int numWorkers) {
//    for (int i = 0; i < numWorkers; i++) {
//        std::thread worker(handleConnections);
//        worker.detach();
//        connectionWorkers.emplace_back(worker);
//    }
//}


