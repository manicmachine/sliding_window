//
// Created by csather on 3/21/21.
//

#include "PacketBuilder.h"

// Taken from rosettacode.org/wiki/CRC-32#C.2B.2B for CRC32 implementation
// Generates a lookup table for the checksums of all 8-bit values.
std::array<std::uint_fast32_t, 256> PacketBuilder::generateCrcLookupTable() {
    auto const reversed_polynomial = std::uint_fast32_t{0xEDB88320uL};

    // This is a function object that calculates the checksum for a value,
    // then increments the value, starting from zero.
    struct byte_checksum {
        std::uint_fast32_t operator()() noexcept
        {
            auto checksum = static_cast<std::uint_fast32_t>(n++);

            for (auto i = 0; i < 8; ++i)
                checksum = (checksum >> 1) ^ ((checksum & 0x1u) ? reversed_polynomial : 0);

            return checksum;
        }

        unsigned n = 0;
    };

    auto table = std::array<std::uint_fast32_t, 256>{};
    std::generate(table.begin(), table.end(), byte_checksum{});

    return table;
}

// Taken from rosettacode.org/wiki/CRC-32#C.2B.2B for CRC32 implementation
template <typename InputIterator>
std::uint_fast32_t PacketBuilder::generateChksum(InputIterator first, InputIterator last) {
    // Generate lookup table only on first use then cache it - this is thread-safe.
    static auto const table = PacketBuilder::generateCrcLookupTable();

    // Calculate the checksum - make sure to clip to 32 bits, for systems that don't
    // have a true (fast) 32-bit type.
    return std::uint_fast32_t{0xFFFFFFFFuL} &
           ~std::accumulate(first, last,
                            ~std::uint_fast32_t{0} & std::uint_fast32_t{0xFFFFFFFFuL},
                            [](std::uint_fast32_t checksum, std::uint_fast8_t value)
                            { return table[(checksum ^ value) & 0xFFu] ^ (checksum >> 8); });
}

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

void PacketBuilder::setChksum(uint_fast32_t chksum) {
    chksum = chksum;
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

struct Packet PacketBuilder::buildPacket() {
    struct Packet pkt{};
    pkt.header.sqn = sqn;
    pkt.header.srcAddr = srcAddr;
    pkt.header.destAddr = destAddr;
    pkt.header.wSize = wSize;

    if (chksum == 0) {
        pkt.header.chksum = generateChksum();
    } else {
        pkt.header.chksum = chksum;
    }

    if (ack) pkt.header.flags.ack = 1;
    if (syn) pkt.header.flags.syn = 1;
    if (fin) pkt.header.flags.fin = 1;

    pkt.payload = payload;

    return pkt;
}