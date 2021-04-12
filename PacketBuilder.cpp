//
// Created by csather on 3/21/21.
//
#include <vector>

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

void PacketBuilder::setPktSize(unsigned int pktSize) {
    pktSize = pktSize;
}

void PacketBuilder::setPayload(string buffer) {
    vector<char> convBuffer(buffer.begin(), buffer.end());
    convBuffer.push_back('\0');
    convBuffer.resize(pktSize, 0);

    payload = &convBuffer[0];
}

int PacketBuilder::generateChksum(Packet pkt, bool broken) {
    boost::crc_32_type chksum;

    if (broken) {
        // Generate a broken checksum by shifting the byte boundaries used by the checksum generator
        chksum.process_bytes(&pkt + 4, sizeof(pkt) - 4);
    } else {
        chksum.process_bytes(&pkt, sizeof(pkt));
    }

    return chksum.checksum();
}

struct Packet PacketBuilder::buildPacket() {
    Packet pkt{};

    pkt.header.srcAddr = srcAddr;
    pkt.header.destAddr = destAddr;
    pkt.header.sqn = sqn;
    pkt.header.wSize = wSize;
    pkt.header.pktSize = pktSize;
    pkt.header.chksum = 0;

    pkt.header.flags.ack = (ack ? '1' : '0');
    pkt.header.flags.syn = (syn ? '1' : '0');
    pkt.header.flags.fin = (fin ? '1' : '0');

    pkt.header.chksum = generateChksum(pkt);

    return pkt;
}
