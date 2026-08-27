#include "tiledecoder.h"
#include <print>

Tile decodeSNES4bppTile(const uint8_t* t)
{
    Tile out;

    for(int row = 0; row < 8; ++row)
    {
        uint8_t b0 = t[row * 2];
        uint8_t b1 = t[row * 2 + 1];
        uint8_t b2 = t[16 + row * 2];
        uint8_t b3 = t[16 + row * 2 + 1];

        for (int bit = 7; bit >= 0; --bit)
        {
            uint8_t p0 = (b0 >> bit) & 1;
            uint8_t p1 = (b1 >> bit) & 1;
            uint8_t p2 = (b2 >> bit) & 1;
            uint8_t p3 = (b3 >> bit) & 1;

            uint8_t color = (p3 << 3) | (p2 << 2) | (p1 << 1) | p0;

            int x = 7 - bit;
            out.pixels[row * 8 + x] = color;
        }
    }

    return out;
}

Tile decodeSNES2bppTile(const uint8_t* t)
{
    Tile out;

    for (int row = 0; row < 8; ++row)
    {
        uint8_t b0 = t[row * 2 + 0]; // plane 0
        uint8_t b1 = t[row * 2 + 1]; // plane 1

        for (int bit = 7; bit >= 0; --bit)
        {
            uint8_t p0 = (b0 >> bit) & 1;
            uint8_t p1 = (b1 >> bit) & 1;

            uint8_t color = (p1 << 1) | p0; // 0–3

            int x = 7 - bit;
            out.pixels[row * 8 + x] = color;
        }
    }

    return out;
}

void encodeSNES2bppTile(uint8_t* out, const Tile& t)
{
    // SNES/NES 2bpp tile = 16 bytes:
    // rows 0–7:
    //   byte 0: plane 0 (bit 0 of each pixel)
    //   byte 1: plane 1 (bit 1 of each pixel)

    for (int row = 0; row < 8; ++row)
    {
        uint8_t p0 = 0;
        uint8_t p1 = 0;

        for (int x = 0; x < 8; ++x)
        {
            uint8_t pix = t.pixels[row * 8 + x] & 0x3; // 0–3 (2bpp)

            int bit = 7 - x; // leftmost pixel → bit 7

            p0 |= ((pix >> 0) & 1) << bit; // plane 0
            p1 |= ((pix >> 1) & 1) << bit; // plane 1
        }

        out[row * 2 + 0] = p0;
        out[row * 2 + 1] = p1;
    }
}

void encodeSNES4bppTile(uint8_t* out, const Tile& t)
{
    // SNES 4bpp tile = 32 bytes:
    // rows 0–7:
    //   byte 0: plane 0 (bit 0 of each pixel)
    //   byte 1: plane 1 (bit 1 of each pixel)
    // rows 8–15:
    //   byte 16: plane 2 (bit 2)
    //   byte 17: plane 3 (bit 3)

    for (int row = 0; row < 8; ++row)
    {
        uint8_t p0 = 0;
        uint8_t p1 = 0;
        uint8_t p2 = 0;
        uint8_t p3 = 0;

        for (int x = 0; x < 8; ++x)
        {
            uint8_t pix = t.pixels[row * 8 + x] & 0xF; // 0–15

            int bit = 7 - x; // SNES stores leftmost pixel in bit 7

            p0 |= ((pix >> 0) & 1) << bit;
            p1 |= ((pix >> 1) & 1) << bit;
            p2 |= ((pix >> 2) & 1) << bit;
            p3 |= ((pix >> 3) & 1) << bit;
        }

        out[row * 2 + 0] = p0;
        out[row * 2 + 1] = p1;
        out[16 + row * 2 + 0] = p2;
        out[16 + row * 2 + 1] = p3;
    }
}

void saveTileToROM(Tile& t, std::vector<uint8_t>& rom, const bool is2bpp)
{
    const size_t size = is2bpp ? 16 : 32;

    std::array<uint8_t, 32> encoded;
    if (is2bpp)
        encodeSNES2bppTile(encoded.data(), t);
    else
        encodeSNES4bppTile(encoded.data(), t);

    std::copy(encoded.begin(), encoded.begin() + size, rom.begin() + t.romAddress);
}


std::vector<Tile> decodeTileRanges(const std::vector<Range>& ranges, const std::vector<uint8_t>& rom, const int tileSize)
{
    std::vector<Tile> out;

    // SNES 4bpp tile = 32 bytes
    // SNES 2bpp tile = 16 bytes

    for(const auto& r : ranges)
    {
        uint32_t pos = r.start;

        while (pos + tileSize <= r.end)
        {
            Tile t;
            if (tileSize == 16)
                t = decodeSNES2bppTile(&rom[pos]);
            else if (tileSize == 32)
                t = decodeSNES4bppTile(&rom[pos]);
            t.romAddress = pos;
            out.push_back(t);
            pos += tileSize;
        }
    }

    return out;
}

std::vector<Tile> decodeTileRange(const Range& range, const std::vector<uint8_t>& rom, const int tileSize)
{
    std::vector<Tile> out;

    // SNES 4bpp tile = 32 bytes
    // SNES 2bpp tile = 16 bytes

    uint32_t pos = range.start;

    while (pos + tileSize <= range.end)
    {
        Tile t;
        if (tileSize == 16)
            t = decodeSNES2bppTile(&rom[pos]);
        else if (tileSize == 32)
            t = decodeSNES4bppTile(&rom[pos]);
        t.romAddress = pos;
        out.push_back(t);
        pos += tileSize;
    }

    return out;
}

void loadMetaTilePalettes(std::vector<MetaTileData>& data, const std::vector<uint8_t>& rom, uint32_t addr, int count)
{
    for (int m = 0; m < count; m++)
    {
        uint8_t b = rom[addr + m];

        uint8_t p0 = (b >> 0) & 0x03;
        uint8_t p1 = (b >> 2) & 0x03;
        uint8_t p2 = (b >> 4) & 0x03;
        uint8_t p3 = (b >> 6) & 0x03;

        data[m].palettes[0] = p0; // TL
        data[m].palettes[1] = p1; // TR
        data[m].palettes[2] = p2; // BL
        data[m].palettes[3] = p3; // BR
    }
}

void saveMetaTilePalettes(std::vector<uint8_t>& rom, uint32_t addr, const std::vector<MetaTileData>& data)
{
    for (size_t m = 0; m < data.size(); ++m)
    {
        const auto& palettes = data[m].palettes;

        uint8_t p0 = palettes[0] & 0x03; // TL
        uint8_t p1 = palettes[1] & 0x03; // TR
        uint8_t p2 = palettes[2] & 0x03; // BL
        uint8_t p3 = palettes[3] & 0x03; // BR

        uint8_t b =
            (p0 << 0) |
            (p1 << 2) |
            (p2 << 4) |
            (p3 << 6);

        rom[addr + m] = b;
    }
}

std::vector<MetaTileData> decodeMetaTile32NES(const std::vector<uint8_t>& rom, uint32_t addr, int count)
{
    std::vector<MetaTileData> out;

    for (int m = 0; m < count; m += 4)
    {
        uint16_t idx[4];
        uint8_t  data[4];

        for (int i = 0; i < 4; ++i)
        {
            uint8_t b = rom[addr + (m + i)];

            data[i] = (b >> 6) & 0x03;
            idx[i] = b & 0x3F;
        }

        MetaTileData mt;

        mt.tileIndexes[0] = idx[0]; // TL
        mt.collision[0] = data[0];

        mt.tileIndexes[1] = idx[2]; // TR
        mt.collision[1] = data[2];

        mt.tileIndexes[2] = idx[1]; // BL
        mt.collision[2] = data[1];

        mt.tileIndexes[3] = idx[3]; // BR
        mt.collision[3] = data[3];

        out.push_back(mt);
    }

    return out;
}

std::vector<MetaTileData> decodeMetaTile32SNES(const std::vector<uint8_t>& rom, uint32_t addr, uint32_t collision, int count)
{
    std::vector<MetaTileData> out;

    for (int m = 0; m < count; m += 4)
    {
        MetaTileData mt;

        mt.tileIndexes[0] = rom[addr + (0 + m)]; // TL
        mt.collision[0] = rom[collision + (0 + m)];

        mt.tileIndexes[1] = rom[addr + (2 + m)]; // TR
        mt.collision[1] = rom[collision + (2 + m)];

        mt.tileIndexes[2] = rom[addr + (1 + m)]; // BL
        mt.collision[2] = rom[collision + (1 + m)];

        mt.tileIndexes[3] = rom[addr + (3 + m)]; // BR
        mt.collision[3] = rom[collision + (3 + m)];

        out.push_back(mt);
    }

    return out;
}

void encodeMetaTile32NES(std::vector<uint8_t>& rom, uint32_t addr, const std::vector<MetaTileData>& metaTiles)
{
    for (size_t m = 0; m < metaTiles.size(); ++m)
    {
        const MetaTileData& mt = metaTiles[m];

        uint8_t b0 = ((mt.collision[0] & 0x03) << 6) | (mt.tileIndexes[0] & 0x3F); // TL
        uint8_t b1 = ((mt.collision[2] & 0x03) << 6) | (mt.tileIndexes[2] & 0x3F); // BL
        uint8_t b2 = ((mt.collision[1] & 0x03) << 6) | (mt.tileIndexes[1] & 0x3F); // TR
        uint8_t b3 = ((mt.collision[3] & 0x03) << 6) | (mt.tileIndexes[3] & 0x3F); // BR

        rom[addr + m * 4 + 0] = b0;
        rom[addr + m * 4 + 1] = b1;
        rom[addr + m * 4 + 2] = b2;
        rom[addr + m * 4 + 3] = b3;
    }
}

void encodeMetaTile32SNES(std::vector<uint8_t>& rom, uint32_t addr, uint32_t collision, const std::vector<MetaTileData>& metaTiles)
{
    for (size_t i = 0; i < metaTiles.size(); ++i)
    {
        size_t m = i * 4;
        const MetaTileData& mt = metaTiles[i];

        rom[addr + (m + 0)] = mt.tileIndexes[0];
        rom[addr + (m + 1)] = mt.tileIndexes[2];
        rom[addr + (m + 2)] = mt.tileIndexes[1];
        rom[addr + (m + 3)] = mt.tileIndexes[3];
        rom[collision + (m + 0)] = mt.collision[0];
        rom[collision + (m + 1)] = mt.collision[2];
        rom[collision + (m + 2)] = mt.collision[1];
        rom[collision + (m + 3)] = mt.collision[3];
    }
}

std::vector<BGTileData> loadBackgroundTileData(std::vector<uint8_t>& rom, uint32_t addr, int count)
{
    std::vector<BGTileData> out;
    for (int i = 0; i < count; i += 2)
    {
        BGTileData td;

        td.tileId = rom[addr + i];
        uint8_t attr = rom[addr + (i + 1)];
        td.vramPage = attr & 0x03;
        td.palette = ((attr >> 2) & 0x07);
        td.highPriority = (attr & 0x20) != 0;
        td.hFlip = (attr & 0x40) != 0;
        td.vFlip = (attr & 0x80) != 0;

        out.push_back(td);
    }
    return out;
}

void saveBackgroundTileData(std::vector<uint8_t>& rom, uint32_t addr, std::vector<BGTileData> data)
{
    for (int i = 0; i < data.size(); ++i)
    {
        const BGTileData& td = data[i];

        rom[addr + i * 2] = td.tileId;

        uint8_t attr = 0;
        attr |= (td.vramPage & 0x03);
        attr |= (td.palette & 0x07) << 2;
        attr |= td.highPriority ? 0x20 : 0;
        attr |= td.hFlip ? 0x40 : 0;
        attr |= td.vFlip ? 0x80 : 0;

        rom[addr + i * 2 + 1] = attr;
    }
}
