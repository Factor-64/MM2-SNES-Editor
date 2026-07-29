#pragma once
#include <vector>

struct SNESHeader {
    char title[22];       // 22 bytes (21 chars + null)
    uint8_t mapMode;      // $25
    uint8_t romType;      // $26
    uint8_t romSize;      // $27
    uint8_t sramSize;     // $28
    uint8_t country;      // $29
    uint8_t license;      // $2A
    uint8_t version;      // $2B
    uint16_t checksum;    // $2C–$2D
    uint16_t checksumComp;// $2E–$2F
};

SNESHeader readSNESHeader(const std::vector<uint8_t>& rom);
