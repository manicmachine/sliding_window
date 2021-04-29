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
//    vector<thread> connectionWorkers;
    queue<Connection> pendingConnections; // Store connections yet to be handled
    ApplicationState *appState;

    Connection createConnection(const string& ipAddress, bool isPing = false);
    Connection createConnection(int sockfd, sockaddr_in clientAddr, sockaddr_in &serverAddr);
    void addToPktBuffer(Connection &connection, Packet pkt);
    void sendPacket(Connection &connection, PacketInfo &pktInfo);
    void sendPacket(Connection &connection, Packet &pkt);
    Packet recPacket(Connection &connection, bool &timeout, bool &badPkt);
    Packet sendAndRec(Connection &connection, PacketInfo &pktInfo, bool &timeout, bool &badPkt);
    Packet recAndAck(Connection &connection, bool &timeout, bool &badPkt);
    timeval toTimeval(chrono::microseconds value);
    bool packetBadLuck(float prob);

public:
    ConnectionController(ApplicationState &appState);
//    static void handleConnections(); // Worker logic
//    void setupWorkers(int numWorkers); // Create workers

    // Initialize Connection objects and place them in pending queue
    void initializeConnections(const string& ipAddress = "");

    // Client - opens connection with server and begins communication
    void handleConnection(Connection &connection, bool isPing = false);
    void handshake(Connection &connection, bool isPing = false);
    void transferFile(Connection &connection);

    chrono::microseconds generatePingBasedTimeout();
    /* Start listening for incoming connections, creating Connection objects as they arrive, and
     * process them
     */
    int startServer();

    // Checks pending connections processes them
    void processConnections();

    void printWindow(Connection &connection);
    void printPacket(Packet &pkt);
    string getLocalAddress();
    string md5(const string& data);

};
#endif //SLIDING_WINDOW_CONNECTIONCONTROLLER_H
