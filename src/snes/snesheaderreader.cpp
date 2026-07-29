#include "snesheaderreader.h"

SNESHeader readSNESHeader(const std::vector<uint8_t>& rom)
{
    SNESHeader h{};
    if(rom.size() < 0x10000)
        return h;

    size_t loBase = 0x7FC0;
    size_t hiBase = 0xFFC0;

    uint8_t loMap = rom[loBase + 0x15];
    uint8_t hiMap = rom[hiBase + 0x15];

    auto valid = [](uint8_t mm)
    {
        uint8_t mode = mm & 0x0F;
        return mode == 0 || mode == 1 || mode == 2 ||
               mode == 3 || mode == 5 || mode == 0x0A;
    };

    size_t base;

    if (valid(hiMap) && !valid(loMap))
        base = hiBase;
    else if (valid(loMap) && !valid(hiMap))
        base = loBase;
    else
        base = hiBase;

    // Title (21 bytes)
    memcpy(h.title, &rom[base], 21);
    h.title[21] = '\0';

    h.mapMode      = rom[base + 0x15];
    h.romType      = rom[base + 0x16];
    h.romSize      = rom[base + 0x17];
    h.sramSize     = rom[base + 0x18];
    h.country      = rom[base + 0x19];
    h.license      = rom[base + 0x1A];
    h.version      = rom[base + 0x1B];

    h.checksumComp = rom[base + 0x1C] | (rom[base + 0x1D] << 8);
    h.checksum     = rom[base + 0x1E] | (rom[base + 0x1F] << 8);

    return h;
}

