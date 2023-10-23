#include "R503Packet.h"

R503Packet::R503Packet(uint16_t length, uint8_t *data) : length(length), payload(data) { calculateChecksum();}

R503Packet::R503Packet(uint8_t pid, uint16_t length, uint8_t *data) : type(pid), length(length), payload(data)
{
    calculateChecksum();
}

uint16_t R503Packet::calculateChecksum()
{
    checksum = type;
    checksum += ((length + 2) >> 8) + ((length + 2)&0xFF);

    int count = 0;

    for (uint16_t i = 0; i < length; i++)
    {
        checksum += payload[i];
        count++;
    }

    return checksum;
}

bool R503Packet::isChecksumValid()
{
    uint16_t original = checksum;

    checksum = calculateChecksum();
    if (original == checksum)
        return true;

    checksum = original;

    return false;
}