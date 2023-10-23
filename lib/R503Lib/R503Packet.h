#ifndef R503PACKET_H
#define R503PACKET_H

#include <Arduino.h>

#define R503_PKT_START_CODE 0xEF01
#define R503_PKT_COMMAND 0x01
#define R503_PKT_DATA_START 0x02
#define R503_PKT_ACK 0x07
#define R503_PKT_DATA_END 0x08

struct R503Packet
{
    uint32_t address;
    uint8_t type;
    uint16_t length;
    uint8_t *payload;
    uint16_t checksum;
    uint8_t *payloadOverflow;

    R503Packet(uint8_t pid, uint16_t length, uint8_t *data);
    R503Packet(uint16_t length, uint8_t *data);

    void computeChecksum();
    uint16_t calculateChecksum();
    bool isChecksumValid();
    bool checksumMatches();
};


#endif