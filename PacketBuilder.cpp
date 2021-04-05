//
// Created by csather on 3/21/21.
//

#include "PacketBuilder.h"

void PacketBuilder::setSqn(unsigned int sqn) {
   sqn = sqn;
}

void PacketBuilder::setSrcAddr(struct sockaddr_in srcAddr) {
    srcAddr = srcAddr;
}

void PacketBuilder::setDestAddr(struct sockaddr_in destAddr) {
    destAddr = destAddr;
}

void PacketBuilder::setWSize(unsigned short wSize) {
    wSize = wSize;
}

void PacketBuilder::enableBrokenChksum() {
    brokenChksum = true;
}

void PacketBuilder::enableAckBit() {
    ack = true;
}

void PacketBuilder::enableSynBit() {
    syn = true;
}

void PacketBuilder::enableFinBit() {
    fin = true;
}

void PacketBuilder::setPayload(char *buffer) {
    payload = buffer;
}

void PacketBuilder::generateChksum(Packet pkt, bool broken) {
    if (broken) {
        // Generate a broken checksum by shifting the byte boundaries used by the checksum generator
        chksum.process_bytes(&pkt + 4, sizeof(pkt) - 4);
    } else {
        chksum.process_bytes(&pkt, sizeof(pkt));
    }
}

struct Packet PacketBuilder::buildPacket() {
    struct Packet pkt{};
    pkt.header.sqn = sqn;
    pkt.header.srcAddr = srcAddr;
    pkt.header.destAddr = destAddr;
    pkt.header.wSize = wSize;

    if (ack) pkt.header.flags.ack = 1;
    if (syn) pkt.header.flags.syn = 1;
    if (fin) pkt.header.flags.fin = 1;

    pkt.payload = payload;
    generateChksum(pkt, brokenChksum);

    return pkt;
}