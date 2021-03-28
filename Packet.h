//
// Created by csather on 3/17/21.
//

#ifndef SLIDING_WINDOW_PACKET_H
#define SLIDING_WINDOW_PACKET_H

using namespace std;

struct Packet {
    struct Header {
        struct sockaddr_in srcAddr; // Packet source information (port, address)
        struct sockaddr_in destAddr; // Packet destination information (port, address)
        unsigned int sqn = 0; // Sequence number - If syn is enabled, the sequence number of the first data byte is this + 1. If ack is enabled, this is the ack number
        unsigned short wSize = 0; // Window size - If syn is enabled, this will be set to synchronize the window size
        uint_fast32_t chksum = 0; // Packet checksum

        struct flags {
            char ack = 0; // Indicates this is an ack packet
            char syn = 0; // Indicates to synchronize sequence numbers
            char fin = 0; // Indicates this is the last packet from sender
        } flags;
    } header;

    char *payload = 0;
};


#endif //SLIDING_WINDOW_PACKET_H
