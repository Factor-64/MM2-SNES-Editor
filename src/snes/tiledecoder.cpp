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

MemoryDelta saveTileToROM(Tile& t, std::vector<uint8_t>& rom, const bool is2bpp)
{
    const size_t size = is2bpp ? 16 : 32;

    std::array<uint8_t, 32> encoded;
    if (is2bpp)
        encodeSNES2bppTile(encoded.data(), t);
    else
        encodeSNES4bppTile(encoded.data(), t);

    MemoryDelta mem;
    mem.address = t.romAddress;
    mem.newData.insert(mem.newData.end(), encoded.begin(), encoded.begin() + size);
    mem.oldData.insert(mem.oldData.end(), rom.begin() + t.romAddress, rom.begin() + t.romAddress + size);
    return mem;
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

MemoryDelta saveBackgroundTileData(std::vector<uint8_t>& rom, uint32_t addr, const BGTileData& td)
{
    MemoryDelta mem;
    mem.address = addr;
    mem.oldData.push_back(rom[addr]);
    mem.oldData.push_back(rom[addr + 1]);
    mem.newData.push_back(td.tileId);
    

    uint8_t attr = 0;
    attr |= (td.vramPage & 0x03);
    attr |= (td.palette & 0x07) << 2;
    attr |= td.highPriority ? 0x20 : 0;
    attr |= td.hFlip ? 0x40 : 0;
    attr |= td.vFlip ? 0x80 : 0;

    mem.newData.push_back(attr);

    return mem;
}

MetaTileData convertToMetaTileData(const MetaTile& mt)
{
    MetaTileData out;

    // TL, TR, BL, BR
    out.tileIndexes[0] = mt.macroIndex[0];
    out.tileIndexes[1] = mt.macroIndex[1];
    out.tileIndexes[2] = mt.macroIndex[2];
    out.tileIndexes[3] = mt.macroIndex[3];

    out.palettes = mt.palettes;
    out.collision = mt.collision;

    return out;
}

MemoryDelta encodeMetaTile32NES(std::vector<uint8_t>& rom, uint32_t addr, const MetaTileData& mt)
{
    std::array<uint8_t, 4> b;
    b[0] = ((mt.collision[0] & 0x03) << 6) | (mt.tileIndexes[0] & 0x3F); // TL
    b[1] = ((mt.collision[2] & 0x03) << 6) | (mt.tileIndexes[2] & 0x3F); // BL
    b[2] = ((mt.collision[1] & 0x03) << 6) | (mt.tileIndexes[1] & 0x3F); // TR
    b[3] = ((mt.collision[3] & 0x03) << 6) | (mt.tileIndexes[3] & 0x3F); // BR

    MemoryDelta m;
    m.address = addr;
    for (int i = 0; i < b.size(); ++i)
    {
        m.newData.push_back(b[i]);
        m.oldData.push_back(rom[addr + i]);
    }
    return m;
}

DataChanged encodeMetaTile32SNES(std::vector<uint8_t>& rom, uint32_t addr, uint32_t collision, MetaTileData& mt)
{
    DataChanged d;
    MemoryDelta m;
    m.address = addr;
    for (int i = 0; i < mt.tileIndexes.size(); ++i)
    {
        m.newData.push_back(mt.tileIndexes[i]);
        m.oldData.push_back(rom[addr + i]);
    }
    d.deltas.push_back(m);
    m.newData.clear();
    m.oldData.clear();
    m.address = collision;
    for (int i = 0; i < mt.collision.size(); ++i)
    {
        m.newData.push_back(mt.collision[i]);
        m.oldData.push_back(rom[collision + i]);
    }
    d.deltas.push_back(m);
    return d;
}

MemoryDelta saveMetaTilePalette(std::vector<uint8_t>& rom, uint32_t addr, const MetaTileData& data)
{
    MemoryDelta m;
    const auto& palettes = data.palettes;

    uint8_t p0 = palettes[0] & 0x03; // TL
    uint8_t p1 = palettes[1] & 0x03; // TR
    uint8_t p2 = palettes[2] & 0x03; // BL
    uint8_t p3 = palettes[3] & 0x03; // BR

    uint8_t b =
        (p0 << 0) |
        (p1 << 2) |
        (p2 << 4) |
        (p3 << 6);

    m.address = addr;
    m.newData.push_back(b);
    m.oldData.push_back(rom[addr]);
    return m;
}

DataChanged saveMetaTileToROM(std::vector<uint8_t>& rom, uint32_t addr, uint32_t paladdr, uint32_t collision, MetaTile& mt)
{
    DataChanged data;
    MemoryDelta m;
    MetaTileData mtd = convertToMetaTileData(mt);

    if (collision != 0)
        data = encodeMetaTile32SNES(rom, addr, collision, mtd);
    else
    {
        m = encodeMetaTile32NES(rom, addr, mtd);
        data.deltas.push_back(m);
    }
    m = saveMetaTilePalette(rom, paladdr, mtd);
    data.deltas.push_back(m);
    return data;
}

std::vector<MetaTile> makeMetaTiles(const std::vector<MetaTileData>& data, const std::vector<MacroTile>& macroTiles)
{
    std::vector<MetaTile> out;
    out.resize(data.size());
    for (size_t i = 0; i < data.size(); ++i)
    {
        for (size_t j = 0; j < data[i].tileIndexes.size(); ++j)
        {
            int idx = data[i].tileIndexes[j];
            if (idx >= macroTiles.size()) continue;
            out[i].tiles[j] = macroTiles[data[i].tileIndexes[j]];
            out[i].macroIndex[j] = idx;
            out[i].palettes[j] = data[i].palettes[j];
            out[i].collision[j] = data[i].collision[j];
        }
    }
    return out;
}
