//
// Created by csather on 4/4/21.
//

#include <arpa/inet.h>
#include <string.h>
#include <cmath>
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
    if (appState->role == CLIENT) connection.pktsSent++; // pktsSent are used for another metric for servers
    pktInfo.count++;

    if (!appState->connectionSettings.damagedPackets.empty() && appState->connectionSettings.damagedPackets.front() == pkt->header.sqn) {
        pkt->header.chksum = ~pkt->header.chksum;
        appState->connectionSettings.damagedPackets.erase(appState->connectionSettings.damagedPackets.begin());
    } else if (packetBadLuck(appState->connectionSettings.damageProb)) {
        pkt->header.chksum = ~pkt->header.chksum;
    } else if (!appState->connectionSettings.lostPackets.empty() && appState->connectionSettings.lostPackets.front() == pkt->header.sqn) {
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
            if (pktInfo.count > 1) {
                connection.resentPkts++;
                printf("Packet %u re-transmitted\n", pkt->header.sqn);
            } else {
                printf("Packet %u sent\n", pkt->header.sqn);
            }
        }

        if (lost) {
            printf("Packet LOST in transmission\n");
        }
    }


    if (appState->role == CLIENT) {
        if (pkt->header.sqn > connection.lastFrame.lastFrameSent) connection.lastFrame.lastFrameSent = pkt->header.sqn;
        pktInfo.timeout = chrono::system_clock::now() + connection.timeoutInterval;
        connection.timeoutQueue.push(&pktInfo);
    }
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
        if (pkt.header.pktSize != 0 && !(pkt.header.flags.syn == 1 && pkt.header.flags.ack == 1)) {
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

    // Server tracks the total number of packets received
    if (appState->role == SERVER) connection.pktsSent++;

    if (!timeout) {
        // Check if damaged
        int chksum = pkt.header.chksum;
        pkt.header.chksum = 0;

        if (chksum != PacketBuilder::generateChksum(pkt)) {
            badPkt = true;
            printf("Checksum FAILED!\n");
        }
    }

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

// Receives a packet from the client and acks it, returning a valid client data packet
Packet ConnectionController::recAndAck(Connection &connection, bool &timeout, bool &badPkt) {
    bool validPkt = false;
    Packet pkt, ackPkt{};
    PacketInfo pktInfo{};

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

        if (pkt.header.sqn <= connection.lastRec.lastFrameRec) connection.resentPkts++;

        // Send ACK
        pktBuilder.setSqn(pkt.header.sqn);
        pktBuilder.enableAckBit();

        if (pkt.header.flags.syn == 1 && pkt.header.flags.ping != 1) {
            pktBuilder.enableSynBit();
            pktBuilder.setPktSize(connection.pktSizeBytes);

            connection.filename = pkt.payload;
        } else if (pkt.header.flags.fin == 1) {
            pktBuilder.enableFinBit();
        } else if (pkt.header.flags.ping == 1) {
            pktBuilder.enablePingBit();
        }

        ackPkt = pktBuilder.buildPacket();
        pktInfo.pkt = &ackPkt;

        sendPacket(connection, pktInfo);

        // No need to return PING or SYN packets, just ACK and continue
        if (pkt.header.flags.ping == 1 || pkt.header.flags.syn == 1) continue;
    } while (!validPkt);

    // Return valid pkt
    return pkt;
}

void ConnectionController::transferFile(Connection &connection) {
    bool timeout = false;
    bool badPkt = false;
    bool finished = false;

    if (appState->role == CLIENT) {
        // Client
        // Read file and send chunks along to server
        char *fileBuffer = new char[connection.pktSizeBytes];
        PacketInfo *pktInfo;
        PacketBuilder pktBuilder;
        pktBuilder.setSrcAddr(connection.srcAddr);
        pktBuilder.setDestAddr(connection.destAddr);
        pktBuilder.setSqnBits(connection.sqnBits);
        pktBuilder.setWSize(connection.wSize);

        do {
            // Fill the window/packet buffer
            if (!finished) {
                for (unsigned int i = (connection.lastFrame.lastFrameSent + 1); i <= (connection.wSize + connection.lastRec.lastAckRec); i++) {
                    // Create data packet
                    Packet newPkt;
                    pktBuilder.setSqn(i);
                    connection.bytesRead = fread(fileBuffer, sizeof(char), connection.pktSizeBytes, connection.file);

                    // If we read less than our packet payload size, then we're on our last packet
                    if (connection.bytesRead < connection.pktSizeBytes) {
                        pktBuilder.setPktSize(connection.bytesRead);
                        pktBuilder.enableFinBit();
                        finished = true;
                    }

                    pktBuilder.setPayload(fileBuffer);
                    newPkt = pktBuilder.buildPacket();
                    addToPktBuffer(connection, newPkt);
                }
            }

            // Remove ACK'd packets from timeout queue
            while(connection.timeoutQueue.front()->acked) {
                connection.timeoutQueue.pop();
            }

            // Get next packet from buffer and send it
            if (connection.timeoutQueue.front()->timeout < chrono::system_clock::now()) {
                // Resend packet
                pktInfo = connection.timeoutQueue.front();
                connection.timeoutQueue.pop();
                printf("Packet %u *** TIMED OUT ***\n", pktInfo->pkt->header.sqn);

                if (pktInfo->count < RETRY) {
                    sendPacket(connection, *pktInfo);
                } else {
                    char convertedIP[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &connection.destAddr.sin_addr, convertedIP, INET_ADDRSTRLEN);
                    printf("Packet %u exceeded RETRY limit. Closing connection to %s...\n", pktInfo->pkt->header.sqn, convertedIP);
                    connection.status = CLOSED;
                    break;
                }
            } else {
                // If no timeouts, send all available packets within window
                while (connection.lastFrame.lastFrameSent <= (connection.wSize + connection.lastRec.lastAckRec)) {
                    pktInfo = &connection.pktBuffer[(connection.lastFrame.lastFrameSent + 1) % connection.wSize];
                    sendPacket(connection, *pktInfo);
                }
            }

            // Rec and process ACKs
            Packet ackPkt;
            ackPkt = recPacket(connection, timeout, badPkt);
            if (!timeout && !badPkt && connection.status == OPEN && ackPkt.header.flags.ack == 1) {
                if (ackPkt.header.sqn == (connection.lastRec.lastAckRec + 1)) connection.lastRec.lastAckRec++;

                // Mark associated packet as ACK'd
                connection.pktBuffer[ackPkt.header.sqn % connection.wSize].acked = true;

                // Check if we now have a series of ack'd packets; if so, update lastackrec
                for (int i = 1; i < connection.wSize; i++) {
                    if (connection.pktBuffer[(connection.lastRec.lastAckRec + i) % connection.wSize].acked) {
                        connection.lastRec.lastAckRec++;
                    } else {
                        break;
                    }
                }

                // If we're finished AND our timeout queue is empty, then we've sent all our packets so close the connection
                if (finished && connection.timeoutQueue.empty()) {
                    close(connection.sockfd);
                    connection.status = COMPLETE;
                    printf("Session successfully terminated\n");
                }
            } else {
                // Something went wrong
                // Timeout - Nothing to do for timeouts as we'll just loop again and resend packets up till RETRY limit
                // Badpkt - Nothing to do for bad packets (ACKs) as we just discard them
                // Connection == Error - Connection
                if (connection.status != OPEN) break;
            }

            printWindow(connection);

        } while(connection.status == OPEN);

        delete[] fileBuffer;
    } else {
        // Server
        Packet pkt;
        PacketInfo pktInfo;
        bool inOrder = false;

        do {
            pkt = recAndAck(connection, timeout, badPkt);

            if (timeout && !finished) {
                // We've exceeded our TTL and haven't finished our transfer
                connection.status = CLOSED;
                break;
            } else if (timeout && finished) {
                connection.status = COMPLETE;
                printf("Session successfully terminated\n");
                break;
            }

            // Need to check for packets at least once more after receiving FIN flag to verify the client received final ACK
            if (pkt.header.flags.fin == 1) finished = true;

            // Check if needs to be buffered (out-of-order) or not and if it's a retransmission
            if (pkt.header.sqn == (connection.lastRec.lastFrameRec + 1)) {
                // In-order packet
                connection.lastRec.lastFrameRec++;
                inOrder = true;
            } else if (connection.pktBuffer[pkt.header.sqn % connection.wSize].pkt->header.sqn == pkt.header.sqn &&
                connection.pktBuffer[pkt.header.sqn % connection.wSize].acked) {
                // Out-of-order but already buffered/acked
                connection.resentPkts++;
            } else if (pkt.header.sqn > (connection.lastRec.lastFrameRec + 1)) {
                // Out-of-order but not buffered/acked yet
                pktInfo = addToPktBuffer(connection, pkt);
                pktInfo.acked = true;
            }

            // Write in-order packet payloads to file
            if (inOrder) {
                fwrite(pkt.payload, sizeof(char), pkt.header.pktSize, connection.file);
            }

            // Check for if we now have a valid sequence buffered, and if so, write them all in series
            for (int i = 1; i < connection.wSize; i++) {
                if ((connection.pktBuffer[(connection.lastRec.lastFrameRec + i) % connection.wSize].acked) &&
                (connection.pktBuffer[(connection.lastRec.lastFrameRec + i) % connection.wSize].pkt->header.sqn > connection.lastRec.lastFrameRec)) {
                    connection.lastRec.lastFrameRec++;
                    fwrite(pkt.payload, sizeof(char), pkt.header.pktSize, connection.file);
                } else {
                    break;
                }
            }

        } while (connection.status == OPEN);
    }

    if (appState->role == CLIENT) {
        double mbps;
        mbps = connection.pktsSent * connection.pktSizeBytes;
        mbps /= pow(10, 6);  // divide by a megabit
        mbps /= chrono::duration_cast<chrono::seconds>(chrono::system_clock::now() - connection.timeConnectionStarted).count();
        mbps *= 8; // Convert from Megabytes-per-second to Megabits-per-second

        printf("Number of original packets sent: %u\n", connection.pktsSent - connection.resentPkts);
        printf("Number of retransmitted packets: %u\n", connection.resentPkts);
        printf("Total elapsed time (ms): %lu\n", chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - connection.timeConnectionStarted).count());

        mbps = connection.pktsSent * connection.pktSizeBytes;
        mbps /= pow(10, 6);  // divide by a megabit
        mbps /= chrono::duration_cast<chrono::seconds>(chrono::system_clock::now() - connection.timeConnectionStarted).count();
        mbps *= 8; // Convert from Megabytes-per-second to Megabits-per-second
        printf("Total throughput (Mbps): %G\n", mbps);

        mbps = (connection.pktsSent - connection.pktSizeBytes) * connection.pktSizeBytes;
        mbps /= pow(10, 6);  // divide by a megabit
        mbps /= chrono::duration_cast<chrono::seconds>(chrono::system_clock::now() - connection.timeConnectionStarted).count();
        mbps *= 8; // Convert from Megabytes-per-second to Megabits-per-second
        printf("Effective throughput (Mbps): %G\n", mbps);
        printf("MD5: %s", md5(appState->filePath).c_str());

    } else {
        printf("Last packet seq # received: %ui", connection.lastRec.lastFrameRec);
        printf("Number of original packets received: %u", connection.pktsSent - connection.resentPkts);
        printf("Number of retransmitted packets received: %u", connection.resentPkts);
        printf("MD5: %s", md5(appState->filePath.append("/").append(connection.filename)).c_str());
    }

    // Cleanup
    fclose(connection.file);
}


PacketInfo ConnectionController::addToPktBuffer(Connection &connection, Packet pkt) {
    PacketInfo pktInfo{};
    pktInfo.pkt = &pkt;

    connection.pktBuffer[pkt.header.sqn % connection.wSize] = pktInfo;
    return pktInfo;
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
        pktBuilder.setSqnBits(connection.sqnBits);
        pktBuilder.setWSize(connection.wSize);
        pktBuilder.setPktSize(connection.pktSizeBytes);

        if (isPing) {
            pktBuilder.setSqn(0);
            pktBuilder.enablePingBit();
        } else {
            pktBuilder.enableSynBit();
            pktBuilder.setSqn(connection.lastFrame.lastFrameSent++);
            pktBuilder.setPayload(appState->fileName);
        }

        Packet synPkt = pktBuilder.buildPacket(), ackPkt{};

        PacketInfo pktInfo{};
        pktInfo.pkt = &synPkt;

        if (isPing) {
            for (int i = 0; i < 5; i++) {
                ackPkt = sendAndRec(connection,pktInfo, timeout, badPkt);
            }
        } else {
            ackPkt = sendAndRec(connection,pktInfo, timeout, badPkt);
        }

        if (connection.status == CLOSED) {
            char convertedIP[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &connection.destAddr.sin_addr, convertedIP, INET_ADDRSTRLEN);
            printf("Unable to establish handshake with %s, Closing connection\n", convertedIP);
            return;
        }

        // Adjust parameters to align with server
        if (!isPing) {
            if (ackPkt.header.wSize != connection.wSize) connection.wSize = ackPkt.header.wSize;
            if (ackPkt.header.pktSize != connection.pktSizeBytes) connection.pktSizeBytes = ackPkt.header.pktSize;
            if (ackPkt.header.sqnBits != connection.sqnBits) connection.sqnBits = ackPkt.header.sqnBits;
            connection.lastRec.lastAckRec = ackPkt.header.sqn;
        }
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

//    for (int i = 0; i < 5; i++) {
//        start = chrono::system_clock::now();
//        Connection connection = createConnection(appState->ipAddresses[0], true);
//        handleConnection(connection, true);
//        avgTimeout = avgTimeout + chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now() - start);
//    }
    start = chrono::system_clock::now();
    Connection connection = createConnection(appState->ipAddresses[0], true);
    handleConnection(connection, true);

    avgTimeout = avgTimeout + chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now() - start);
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

void ConnectionController::printWindow(Connection &connection) {
    printf("Current window = [");
    for (int i = 0; i < connection.pktBuffer.size(); i++) {
        if (i < connection.pktBuffer.size() - 1) {
            printf("%u, ", connection.pktBuffer[i].pkt->header.sqn);
        } else {
            printf("%u", connection.pktBuffer[i].pkt->header.sqn);
        }
    }

    printf("]\n");
}

// Invoke local md5sum to get digital signature of provided filePath
string ConnectionController::md5(const string& data) {
    string command = "/usr/bin/md5sum " + data;
    FILE *pipe = popen(command.c_str(), "r");

    if (pipe == NULL) {
        fprintf(stderr, "Error opening pipe\nError #: %d\n", errno);
        exit(1);
    }

    char buffer[64];
    string md5;

    while (!feof(pipe)) {
        if (fgets(buffer, 64, pipe) != NULL) {
            md5 += buffer;
        }
    }

    md5 = md5.substr(0, 33);
    pclose(pipe);

    return md5;
}

//void ConnectionController::setupWorkers(int numWorkers) {
//    for (int i = 0; i < numWorkers; i++) {
//        std::thread worker(handleConnections);
//        worker.detach();
//        connectionWorkers.emplace_back(worker);
//    }
//}


