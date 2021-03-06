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

void PacketBuilder::resetFlags() {
    this->ack = false;
    this->syn = false;
    this->fin = false;
    this->ping = false;
}

void PacketBuilder::setPktSize(unsigned int pktSize) {
    this->pktSize = pktSize;
}

void PacketBuilder::emptyPayload() {
    if (this->payload == NULL) {
        initPayload();
    }

    bzero(this->payload, this->pktSize);
}

void PacketBuilder::setPayload(const char *buffer, int buffLen) {
    if (this->payload == NULL) {
        initPayload();
    }

//    vector<char> convBuffer(buffer.begin(), buffer.end());
//    convBuffer.resize(this->pktSize, 0);

    bzero(this->payload, this->pktSize);
    if (buffLen > 0){
        memcpy(this->payload, &buffer[0], buffLen);
    } else {
        memcpy(this->payload, &buffer[0], this->pktSize);
    }
}

int PacketBuilder::generateChksum(Packet *pkt) {
    boost::crc_32_type chksum;

    chksum.process_bytes(&pkt->header, sizeof(Packet::Header));

    if (pkt->header.pktSize != 0 && pkt->header.flags.ping != 1 && (pkt->header.flags.syn != 1 && pkt->header.flags.ack !=1)) {
        chksum.process_bytes(pkt->payload, pkt->header.pktSize);
    }

    return chksum.checksum();
}

struct Packet PacketBuilder::buildPacket() {
    this->pkt = new Packet();

    pkt->header.srcAddr = srcAddr;
    pkt->header.destAddr = destAddr;
    pkt->header.sqn = sqn;
    pkt->header.sqnBits = sqnbits;
    pkt->header.wSize = wSize;
    pkt->header.pktSize = pktSize;
    pkt->header.chksum = 0;

    pkt->header.flags.ack = (ack ? 1 : 0);
    pkt->header.flags.syn = (syn ? 1 : 0);
    pkt->header.flags.fin = (fin ? 1 : 0);
    pkt->header.flags.ping = (ping ? 1 : 0);

    if (this->payload == NULL) initPayload(); // Initialize the builders payload
    pkt->initPayload(); // Initialize the packets payload
    memcpy(pkt->payload, this->payload, this->pktSize);
    pkt->header.chksum = generateChksum(this->pkt);

    return *pkt;
}

