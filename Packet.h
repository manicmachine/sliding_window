//
// Created by csather on 3/17/21.
//

#ifndef SLIDING_WINDOW_PACKET_H
#define SLIDING_WINDOW_PACKET_H

#include "boost/crc.hpp"

using namespace std;

struct Packet {

    struct Header {
        Header() {
            this->flags = *new Flags{};
        }

        struct sockaddr_in srcAddr; // Packet source information (port, address)
        struct sockaddr_in destAddr; // Packet destination information (port, address)
        unsigned int sqn = 0; // Sequence number - If syn is enabled, the sequence number of the first data byte is this + 1. If ack is enabled, this is the ack number
        unsigned int sqnBits = 0; // Sequence range - If syn is enabled, this will be set to synchronize the sequence range
        unsigned short wSize = 0; // Window size - If syn is enabled, this will be set to synchronize the window size
        unsigned int pktSize = 0; // Size of payload (in bytes) - If sync is enabled, this will be used synchronized packet size
        int chksum; // Packet checksum

        struct Flags {
            char ack = 0; // Indicates this is an ack packet
            char syn = 0; // Indicates to synchronize sequence numbers
            char fin = 0; // Indicates this is the last packet from sender
            char ping = 0; // Indicates this packet is for establishing ping-based timeout and has no viable payload
        } flags;
    } header;

    char *payload = NULL;

    Packet() {
        this->header = *new Header{};
    }

    void initPayload() {
        if (this->header.pktSize != 0) {
            this->payload = new char[this->header.pktSize];
        }
    }
};


#endif //SLIDING_WINDOW_PACKET_H
