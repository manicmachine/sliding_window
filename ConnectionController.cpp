//
// Created by csather on 4/4/21.
//

#include <arpa/inet.h>
#include <string.h>
#include <cmath>
#include <thread>

#include "ConnectionController.h"
#include "PacketBuilder.h"

#define PING_ATTEMPTS 3
#define PING_TIMEOUT_SECONDS 5

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
        printf("Awaiting connection...\n");
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
    for (auto &ipAddress : appState->ipAddresses) {
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
        connection.pktBuffer = new PacketInfo[connection.wSize];
        connection.timeoutInterval = appState->connectionSettings.timeoutInterval;
        connection.file = std::fopen(appState->filePath.c_str(), "rb");
    } else {
        connection.timeoutInterval = chrono::seconds(PING_TIMEOUT_SECONDS);
    }

    // Set socket timeout interval
    timeval timeout = toTimeval(connection.timeoutInterval);
    if (setsockopt(connection.sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        fprintf(stderr, "Setting socket options failed\nError #: %d\n", errno);
        connection.status = ERROR;
    }

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
    connection.pktBuffer = new PacketInfo[connection.wSize];

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
            port = htons(connection.destAddr.sin_port);

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
    int originalChksum = pktInfo.pkt.header.chksum;
    bool lost = false, damaged = false;
    if (appState->role == CLIENT) connection.pktsSent++; // pktsSent are used for another metric for servers
    pktInfo.count++;

    if (pktInfo.pkt.header.flags.ping != 1) {
        if (!appState->connectionSettings.damagedPackets.empty() && appState->connectionSettings.damagedPackets.front() == pktInfo.pkt.header.sqn) {
            pktInfo.pkt.header.chksum = ~pktInfo.pkt.header.chksum;
            appState->connectionSettings.damagedPackets.erase(appState->connectionSettings.damagedPackets.begin());
            damaged = true;
        } else if (packetBadLuck(appState->connectionSettings.damageProb) && pktInfo.count < (RETRY - 1)) {
            pktInfo.pkt.header.chksum = ~pktInfo.pkt.header.chksum;
            damaged = true;
        } else if (!appState->connectionSettings.lostPackets.empty() && appState->connectionSettings.lostPackets.front() == pktInfo.pkt.header.sqn) {
            lost = true;
            appState->connectionSettings.lostPackets.erase(appState->connectionSettings.lostPackets.begin());
        } else if (packetBadLuck(appState->connectionSettings.lostProb) && pktInfo.count < (RETRY - 1)) {
            lost = true;
        }
    }

    // Signal this is our last ping
    if (pktInfo.count == RETRY && pktInfo.pkt.header.flags.ping == 1) {
        pktInfo.pkt.header.flags.fin = 1;
        pktInfo.pkt.header.chksum = 0;
        pktInfo.pkt.header.chksum = PacketBuilder::generateChksum(&pktInfo.pkt);
    }

    if (!lost) {
        // Send header
        if (write(connection.sockfd, (Packet::Header *) &(pktInfo.pkt.header), sizeof(Packet::Header)) < 0) {
            fprintf(stderr, "Error writing header to socket\nError #: %d\n", errno);
            connection.status = ERROR;

            return;
        }

        if (pktInfo.pkt.header.pktSize != 0 && !(pktInfo.pkt.header.flags.syn == 1 && pktInfo.pkt.header.flags.ack == 1)) {
            // Send payload
            if (write(connection.sockfd, (void *) pktInfo.pkt.payload, pktInfo.pkt.header.pktSize) < 0) {
                fprintf(stderr, "Error writing payload to socket\nError #: %d\n", errno);
                connection.status = ERROR;

                return;
            }
        }
    }

    // Repair "damage"
    if (damaged) pktInfo.pkt.header.chksum = originalChksum;

    if (appState->verbose) {
        if (pktInfo.pkt.header.flags.ack == 1) {
            printf("Ack %u sent\n", pktInfo.pkt.header.sqn);
        } else {
            if (pktInfo.count > 1) {
                connection.resentPkts++;
                printf("Packet %u re-transmitted\n", pktInfo.pkt.header.sqn);
            } else {
                printf("Packet %u sent\n", pktInfo.pkt.header.sqn);
            }
        }

        if (lost) {
            printf("Packet LOST in transmission\n");
        }
    }

    if (appState->role == CLIENT) {
        if (pktInfo.pkt.header.sqn > connection.lastFrame.lastFrameSent) connection.lastFrame.lastFrameSent = pktInfo.pkt.header.sqn;
        pktInfo.timeout = chrono::system_clock::now() + connection.timeoutInterval;
        if (pktInfo.pkt.header.flags.ping != 1 && pktInfo.pkt.header.flags.syn != 1) connection.timeoutQueue.push(&pktInfo);
    }
}

Packet ConnectionController::recPacket(Connection &connection, bool &timeout, bool &badPkt) {
    auto *pkt = new Packet();
    timeout = false;
    badPkt = false;

    // Listen for response
    // Read header
    connection.bytesRead = read(connection.sockfd, (Packet::Header *) &(pkt->header), sizeof(Packet::Header));
    if (connection.bytesRead < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // No packets received within timeout interval
            timeout = true;
            printf("Timed out waiting for packet\n");
        } else {
            fprintf(stderr, "Error reading header from socket\nError #: %d\n", errno);
            connection.status = ERROR;
            return *pkt;
        }
    } else if (connection.bytesRead == 0) {
        // Nothing available in the socket so we created an empty packet
        timeout = true;
    } else {
        // Don't initialize packet payload if pktsize == 0 (no payload) or it's a SYN/ACK packet (pktsize in negotiation)
        if (pkt->header.pktSize != 0 && !(pkt->header.flags.syn == 1 && pkt->header.flags.ack == 1)) {
            pkt->initPayload();

            // Read payload
            if (read(connection.sockfd, (void *) pkt->payload, pkt->header.pktSize) < 0) {
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
            if (appState->role == CLIENT && pkt->header.flags.ack == 1 && !timeout) {
                printf("Ack %u received\n", pkt->header.sqn);
            } else {
                printf("Packet %u received\n", pkt->header.sqn);
            }
        }
    }

    if (timeout) return *pkt;

    // Server tracks the total number of packets received
    if (appState->role == SERVER && pkt->header.flags.ping != 1) connection.pktsSent++;


    // Check if damaged
    int chksum = pkt->header.chksum;
    pkt->header.chksum = 0;

    if (chksum != PacketBuilder::generateChksum(pkt)) {
        badPkt = true;
        printf("Checksum FAILED!\n");
        // Assume this broken packet will be resent so increment counter
        if (appState->role == SERVER) connection.resentPkts++;
    }

    // TODO: REMOVE ME!
    printPacket(*pkt);
    //
    return *pkt;
}

// Implements full round trip communication between client/server, returning the ack'd packet
Packet ConnectionController::sendAndRec(Connection &connection, PacketInfo &pktInfo, bool &timeout, bool &badPkt) {
    auto *pkt = new Packet();
    bool acked = false;

    // Resend packet up to RETRY times or until ack'd
    // TODO: Should probably utilize the timeout queue instead of a while loop
    while (pktInfo.count < RETRY && pkt->header.flags.ack != 1) {
        sendPacket(connection, pktInfo);
        *pkt = recPacket(connection, timeout, badPkt);

        if (pkt->header.flags.ack == 1 && pkt->header.sqn == pktInfo.pkt.header.sqn) {
            acked = true;
            break;
        }

        if (badPkt && pktInfo.count < RETRY) {
            badPkt = false;
            continue;
        }
    }

    // We've exceeded retry limit but haven't received an ACK so close connection
    if (!acked || badPkt || timeout || (pktInfo.pkt.header.flags.syn == 1 && pkt->header.flags.syn != 1)) connection.status = CLOSED;

    return *pkt;
}

// Receives a packet from the client and acks it, returning a valid client data packet
Packet ConnectionController::recAndAck(Connection &connection, bool &timeout, bool &badPkt) {
    bool validPkt = false;
    bool startSyn = true;
    Packet pkt, ackPkt{};
    PacketInfo pktInfo{};

    PacketBuilder pktBuilder;
    pktBuilder.setSrcAddr(connection.srcAddr);
    pktBuilder.setDestAddr(connection.destAddr);
    pktBuilder.setWSize(connection.wSize);
    pktBuilder.setSqnBits(connection.sqnBits);

     do {
         pktBuilder.resetFlags();
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

        // Toggle flag tracking if we're just starting to sync. If sync packet is sent again (lost/damaged), then
        // increment resent packet counter
        if (pkt.header.sqn <= connection.lastRec.lastFrameRec && !startSyn) {
            connection.resentPkts++;
        } else {
            if (pkt.header.flags.ping != 1) startSyn = false;
        }

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
        pktInfo.pkt = ackPkt;

        sendPacket(connection, pktInfo);

        // No need to return PING or SYN packets, just ACK and continue
        if ((pkt.header.flags.ping == 1 && pkt.header.flags.fin != 1) || pkt.header.flags.syn == 1) continue;
        if (pkt.header.flags.ping == 1 && pkt.header.flags.fin == 1) break;
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
        PacketBuilder pktBuilder;
        pktBuilder.setSrcAddr(connection.srcAddr);
        pktBuilder.setDestAddr(connection.destAddr);
        pktBuilder.setSqnBits(connection.sqnBits);
        pktBuilder.setPktSize(connection.pktSizeBytes);
        pktBuilder.setWSize(connection.wSize);

        do {
            // Fill the window/packet buffer
            if (!finished) {
                for (unsigned int i = (connection.lastFrame.lastFrameSent + 1); i <= (connection.wSize + connection.lastRec.lastAckRec); i++) {
                    // Create data packet
                    pktBuilder.setSqn(i);
                    bzero(fileBuffer, connection.pktSizeBytes);
                    connection.bytesRead = fread(fileBuffer, sizeof(char), connection.pktSizeBytes, connection.file);

                    // If we read less than our packet payload size, then we're on our last packet
                    if (connection.bytesRead < connection.pktSizeBytes) {
                        pktBuilder.setPktSize(connection.bytesRead);
                        pktBuilder.enableFinBit();
                        finished = true;
                    }

                    pktBuilder.setPayload(fileBuffer);
                    Packet newPkt = pktBuilder.buildPacket();
                    addToPktBuffer(connection, newPkt);

                    if (finished) break;
                }
            }

            printWindow(connection);

            // Get next packet from buffer and send it
            if (!connection.timeoutQueue.empty() && connection.timeoutQueue.front()->timeout < chrono::system_clock::now() && !connection.timeoutQueue.front()->acked) {
                // Resend packet
                PacketInfo *pktInfo = connection.timeoutQueue.front();
                connection.timeoutQueue.pop();
                printf("Packet %u *** TIMED OUT ***\n", pktInfo->pkt.header.sqn);
                timeout = false; // reset flag

                if (pktInfo->count < RETRY) {
                    sendPacket(connection, *pktInfo);
                } else {
                    char convertedIP[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &connection.destAddr.sin_addr, convertedIP, INET_ADDRSTRLEN);
                    printf("Packet %u exceeded RETRY limit. Closing connection to %s...\n", pktInfo->pkt.header.sqn, convertedIP);
                    connection.status = CLOSED;
                    break;
                }
            }

            // Send packets in window
            for (unsigned int i = connection.lastFrame.lastFrameSent + 1; i <= (connection.wSize + connection.lastRec.lastAckRec); i++) {
                if (connection.pktBuffer[i % connection.wSize].pkt.header.sqn <= connection.lastFrame.lastFrameSent) break;
                if (!connection.pktBuffer[i % connection.wSize].acked) sendPacket(connection, connection.pktBuffer[i % connection.wSize]);
            }

            // Rec and process ACKs
            Packet ackPkt;
            ackPkt = recPacket(connection, timeout, badPkt);
            if (!timeout && !badPkt && ackPkt.header.flags.ack == 1) {
                if (ackPkt.header.sqn == (connection.lastRec.lastAckRec + 1)) connection.lastRec.lastAckRec++;

                // Mark associated packet as ACK'd
                connection.pktBuffer[ackPkt.header.sqn % connection.wSize].acked = true;

                // Remove ACK'd packets from timeout queue
                while(!connection.timeoutQueue.empty() && connection.timeoutQueue.front()->acked) {
                    connection.timeoutQueue.pop();
                }

                while (!connection.timeoutQueue.empty() && connection.timeoutQueue.front()->pkt.header.sqn == ackPkt.header.sqn) {
                    printf("Ack'd Packet detected in queue; removing\n");
                    connection.timeoutQueue.pop();
                    printf("Queue size: %lu\n", connection.timeoutQueue.size());
                }

                // Check if we now have a series of ack'd packets; if so, update lastackrec
                bool sequence = false;
                int sequenceNum = 0;
                for (int i = 1; i < connection.wSize; i++) {
                    if (connection.pktBuffer[(connection.lastRec.lastAckRec + i) % connection.wSize].acked &&
                            (connection.pktBuffer[(connection.lastRec.lastAckRec + i) % connection.wSize].pkt.header.sqn > connection.lastRec.lastAckRec)) {
                        sequence = true;
                        sequenceNum++;
                    } else {
                        break;
                    }
                }

                if (sequence) {
                    connection.lastRec.lastAckRec += sequenceNum;
                }

                // If we're finished AND our timeout queue is empty, then we've sent all our packets so close the connection
                if (finished && connection.lastRec.lastAckRec == connection.lastFrame.lastFrameSent) {
                    close(connection.sockfd);
                    connection.status = COMPLETE;
                    printf("Session successfully terminated\n");
                }
            } else {
                // Something went wrong
                // Timeout - Nothing to do for timeouts as we'll just loop again and resend packets up till RETRY limit
                // Badpkt - Nothing to do for bad packets (ACKs) as we just discard them
                if (connection.status != OPEN) break;
            }

        } while(connection.status == OPEN);
    } else if (appState->role == SERVER && connection.status == OPEN) {
        // Server
        Packet pkt;
        PacketInfo pktInfo;
        unsigned int finalSqn = 0;
        bool inOrder = false;

        do {
            inOrder = false;

            if (connection.lastRec.lastFrameRec == finalSqn) {
                finished = true;
                connection.status = COMPLETE;
            }

            pkt = recAndAck(connection, timeout, badPkt);

            if (timeout && !finished) {
                // We've exceeded our TTL and haven't finished our transfer
                printf("Connection closed\n");
                connection.status = CLOSED;
                break;
            } else if (timeout && finished) {
                printf("Session successfully terminated\n");
                break;
            }

            // If we received a FIN packet, then we know what our last frame should be
            if (pkt.header.flags.fin == 1) {
                finalSqn = pkt.header.sqn;
            }

            // Check if needs to be buffered (out-of-order) or not and if it's a retransmission
            if (pkt.header.sqn == (connection.lastRec.lastFrameRec + 1)) {
                // In-order packet

                connection.lastRec.lastFrameRec++;
                inOrder = true;
            } else if (connection.pktBuffer[pkt.header.sqn % connection.wSize].pkt.header.sqn == pkt.header.sqn &&
                connection.pktBuffer[pkt.header.sqn % connection.wSize].acked) {
                // Out-of-order but already buffered/acked
                connection.resentPkts++;
            } else if (pkt.header.sqn > (connection.lastRec.lastFrameRec + 1)) {
                // Out-of-order but not buffered/acked yet
                addToPktBuffer(connection, pkt);
                connection.pktBuffer[pkt.header.sqn % connection.wSize].acked = true;
            }

            // Write in-order packet payloads to file
            if (inOrder) {

                if (pkt.header.pktSize > 0) {
                    fwrite(pkt.payload, sizeof(char), pkt.header.pktSize, connection.file);
                }
            }

            // Check for if we now have a valid sequence buffered, and if so, write them all in series
            bool sequence = false;
            int sequenceNum = 0;
            for (int i = 1; i < connection.wSize; i++) {
                if ((connection.pktBuffer[(connection.lastRec.lastFrameRec + i) % connection.wSize].acked) &&
                (connection.pktBuffer[(connection.lastRec.lastFrameRec + i) % connection.wSize].pkt.header.sqn > connection.lastRec.lastFrameRec)) {
                    if (pkt.header.pktSize > 0) {
                        printf("Writing Packet %u payload to disk\n", connection.pktBuffer[(connection.lastRec.lastFrameRec + i) % connection.wSize].pkt.header.sqn );
                        fwrite(connection.pktBuffer[(connection.lastRec.lastFrameRec + i) % connection.wSize].pkt.payload, sizeof(char), pkt.header.pktSize, connection.file);
                    }

                    sequence = true;
                    sequenceNum++;
                } else {
                    break;
                }
            }

            if (sequence) {
                printf("Number of packets in sequence: %i\n", sequenceNum);
                connection.lastRec.lastFrameRec += sequenceNum;
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

        mbps = (connection.pktsSent - connection.resentPkts) * connection.pktSizeBytes;
        mbps /= pow(10, 6);  // divide by a megabit
        mbps /= chrono::duration_cast<chrono::seconds>(chrono::system_clock::now() - connection.timeConnectionStarted).count();
        mbps *= 8; // Convert from Megabytes-per-second to Megabits-per-second
        printf("Effective throughput (Mbps): %G\n", mbps);
    } else {
        printf("Last packet seq # received: %u\n", connection.lastRec.lastFrameRec);
        printf("Number of original packets received: %u\n", connection.pktsSent - connection.resentPkts);
        printf("Number of retransmitted packets received: %u\n", connection.resentPkts);
    }

    // Cleanup
    fclose(connection.file);

    if (appState->role == CLIENT) printf("MD5: %s\n", md5(appState->filePath).c_str());
    else printf("MD5: %s\n", md5(connection.filename).c_str());
}


void ConnectionController::addToPktBuffer(Connection &connection, Packet pkt) {
    auto pktInfo = new PacketInfo();
    pktInfo->pkt = pkt;

//    //TODO: REMOVE ME!
//    printf("---\n");
//    printf("Inserting Packet SQN %u\n", pkt.header.sqn);
//    printf("Inserted into Pkt Buffer at %u\n", pkt.header.sqn % connection.wSize);
//    printf("PacketInfo size: %lu\n", sizeof(pktInfo));
//    printf("Packet size: %lu\n", sizeof(pkt));
//    printf("Packet Header Sizse: %lu\n", sizeof(Packet::Header));
//    printf("Packet Payload Size: %lu\n", sizeof(pkt.payload));
//    printf("---\n");
//    //

    connection.pktBuffer[pkt.header.sqn % connection.wSize] = *pktInfo;
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
            pktBuilder.setSqn(connection.lastFrame.lastFrameSent);
            pktBuilder.setPayload(appState->fileName.c_str(), appState->fileName.length());
        }

        Packet pkt = pktBuilder.buildPacket(), ackPkt{};

        PacketInfo pktInfo{};
        pktInfo.pkt = pkt;

        if (isPing) {
            bool pingTimeout = true;
            for (int i = 0; i < PING_ATTEMPTS; i++) {
                ackPkt = sendAndRec(connection,pktInfo, timeout, badPkt);

                // Need to check if we get at least 1 valid connection, otherwise the results aren't valid
                if (!timeout) pingTimeout = false;
            }

            // We're done pinging so close the connection
            if (pingTimeout) {
                connection.status = ERROR;
            } else {
                connection.status = CLOSED;
            }

            close(connection.sockfd);
        } else {
            ackPkt = sendAndRec(connection,pktInfo, timeout, badPkt);
        }

        if (connection.status != OPEN && !isPing) {
            char convertedIP[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &connection.destAddr.sin_addr, convertedIP, INET_ADDRSTRLEN);
            printf("Unable to establish handshake with %s, Closing connection\n", convertedIP);
            return;
        }

        // Adjust parameters to align with server
        if (!isPing) {
            if (ackPkt.header.wSize != connection.wSize) {
                if(ackPkt.header.wSize > connection.wSize) {
                    free(connection.pktBuffer);
                    connection.pktBuffer = new PacketInfo[connection.wSize];
                }
                connection.wSize = ackPkt.header.wSize;
            }
            if (ackPkt.header.pktSize != connection.pktSizeBytes) connection.pktSizeBytes = ackPkt.header.pktSize;
            if (ackPkt.header.sqnBits != connection.sqnBits) connection.sqnBits = ackPkt.header.sqnBits;
            connection.lastRec.lastAckRec = ackPkt.header.sqn;
        }
    } else {
        // Server
        // Wait for SYN packet and respond with SYN/ACK
        // recAndAck takes care of responding to ping packets, so returned packet is first data packet
        Packet pkt = recAndAck(connection, timeout, badPkt);

        // Connection was a ping and now it's done so close the connection
        if (pkt.header.flags.ping == 1 && pkt.header.flags.fin == 1) {
            close(connection.sockfd);
            return;
        }

        connection.lastRec.lastFrameRec = pkt.header.sqn;
        connection.filename = appState->filePath + connection.filename;
        connection.file = std::fopen(connection.filename.c_str(), "wb+");

        // Create file with first data packet received before moving to transfer phase
        if (pkt.header.pktSize > 0) fwrite(pkt.payload, sizeof(char), pkt.header.pktSize, connection.file);
    }

    // Both
    // If not Ping, transition to transfer phase
    if (connection.status == OPEN || connection.status == COMPLETE) transferFile(connection);
}

chrono::microseconds ConnectionController::generatePingBasedTimeout() {
    // If we're generating a timeout for a localhost connection, just set to 1 second to avoid excessively small timeouts
    if (appState->role == CLIENT && appState->ipAddresses[0].compare("127.0.0.1") ==0) {
        if (appState->verbose) printf("Detected localhost ping-based timeout test; configuring to 1 second timeout\n");
        return chrono::seconds(1);
    }

    chrono::microseconds avgTimeout = chrono::microseconds (0); // ms
    int scalar = appState->connectionSettings.timeoutScale * 100; // Issues happen when multiplying chrono units with floats, so make int first then divide by 10

    Connection connection = createConnection(appState->ipAddresses[0], true);
    handleConnection(connection, true);

    if (connection.status == CLOSED) {
        avgTimeout = chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now() - connection.timeConnectionStarted);
        avgTimeout /= PING_ATTEMPTS;
        avgTimeout = chrono::duration_cast<chrono::microseconds>((avgTimeout * scalar) / 100);
    } else {
        printf("Unable to establish contact with Server; Aborting connection\n");
        avgTimeout = chrono::microseconds (0);
    }

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

    for (int i = connection.lastRec.lastAckRec + 1; i <= (connection.wSize + connection.lastRec.lastAckRec); i++) {
        if (!(connection.pktBuffer[i % connection.wSize].pkt.header.sqn > connection.lastRec.lastAckRec)) break;

        if (i <= (connection.wSize + connection.lastRec.lastAckRec) - 1 && (connection.pktBuffer[i % connection.wSize].pkt.header.sqn < connection.pktBuffer[(i + 1) % connection.wSize].pkt.header.sqn)) {
            printf("%u, ", connection.pktBuffer[i % connection.wSize].pkt.header.sqn);
        } else {
            printf("%u", connection.pktBuffer[i % connection.wSize].pkt.header.sqn);
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

void ConnectionController::printPacket(Packet &pkt) {
    printf("DEBUG: Packet SQN: %u\n", pkt.header.sqn);
    printf("DEBUG: Packet PKT Size: %u\n", pkt.header.pktSize);
    printf("DEBUG: Packet Checksum: %i\n", pkt.header.chksum);
    printf("DEBUG: Packet Flags: ");
    if (pkt.header.flags.ack == 1) printf("ACK ");
    if (pkt.header.flags.syn == 1) printf("SYN ");
    if (pkt.header.flags.fin == 1) printf("FIN ");
    if (pkt.header.flags.ping == 1) printf("PING ");
    printf("\n");
//    if (pkt.header.pktSize != 0 && !(pkt.header.flags.syn == 1 && pkt.header.flags.ack == 1) && pkt.header.flags.ping != 1) printf("DEBUG: Packet Payload: %s\n", pkt.payload);
    printf("-----\n");
}

//void ConnectionController::setupWorkers(int numWorkers) {
//    for (int i = 0; i < numWorkers; i++) {
//        std::thread worker(handleConnections);
//        worker.detach();
//        connectionWorkers.emplace_back(worker);
//    }
//}


