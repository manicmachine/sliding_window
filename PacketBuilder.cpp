//
// Created by csather on 3/21/21.
//
#include <vector>

#include "PacketBuilder.h"

void PacketBuilder::initPayload() {
    this->payload = (char *) calloc(1, this->pktSize);
}


void PacketBuilder::setSqn(unsigned int sqn) {
   this->sqn = sqn;
}

void PacketBuilder::setSrcAddr(struct sockaddr_in srcAddr) {
    this->srcAddr = srcAddr;
}

void PacketBuilder::setDestAddr(struct sockaddr_in destAddr) {
    this->destAddr = destAddr;
}

void PacketBuilder::setWSize(unsigned short wSize) {
    this->wSize = wSize;
}

void PacketBuilder::setSqnBits(unsigned char bits) {
    this->sqnbits = bits;
}

void PacketBuilder::enableAckBit() {
    this->ack = true;
}

void PacketBuilder::enableSynBit() {
    this->syn = true;
}

void PacketBuilder::enableFinBit() {
    this->fin = true;
}

void PacketBuilder::enablePingBit() {
    this->ping = true;
}

void PacketBuilder::setPktSize(unsigned int pktSize) {
    this->pktSize = pktSize;
}

void PacketBuilder::setPayload(string buffer) {
    if (this->payload == NULL) {
        initPayload();
    }

    vector<char> convBuffer(buffer.begin(), buffer.end());
    convBuffer.resize(this->pktSize, 0);

    memcpy(this->payload, &convBuffer[0], this->pktSize);
}

int PacketBuilder::generateChksum(Packet pkt) {
    boost::crc_32_type chksum;

    chksum.process_bytes(&pkt.header, sizeof(Packet::Header));
    chksum.process_bytes(pkt.payload, pkt.header.pktSize);

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

    pkt.header.flags.ack = (ack ? 1 : 0);
    pkt.header.flags.syn = (syn ? 1 : 0);
    pkt.header.flags.fin = (fin ? 1 : 0);
    pkt.header.flags.ping = (ping ? 1 : 0);

    if (this->payload == NULL) initPayload();
    pkt.payload = this->payload;
    pkt.header.chksum = generateChksum(pkt);

    return pkt;
}

