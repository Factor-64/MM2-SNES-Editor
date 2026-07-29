#include "cgramdecoder.h"
#include "address.h"
#include <iostream>

static ColorRGBA decodeSNESColor(uint16_t raw)
{
    uint8_t r = (raw & 0x1F) << 3;
    uint8_t g = ((raw >> 5) & 0x1F) << 3;
    uint8_t b = ((raw >> 10) & 0x1F) << 3;

    return { r, g, b, 255 };
}

Palettes decodeCGRAMPalettes(const std::vector<uint8_t>& rom, uint32_t addr, int paletteCount)
{
    Palettes out;

    for (int p = 0; p < paletteCount; ++p)
    {
        Palette pal{};
        for (int i = 0; i < 16; ++i)
        {
            uint32_t offset = addr + (p * 16 + i) * 2;
            uint16_t raw = rom[offset] | (rom[offset + 1] << 8);
            pal[i] = decodeSNESColor(raw);
        }
        out[p] = pal;
    }

    return out;
}

uint16_t encodeSNESColor(const ColorRGBA& c)
{
    uint16_t r = (c.r >> 3) & 0x1F;
    uint16_t g = (c.g >> 3) & 0x1F;
    uint16_t b = (c.b >> 3) & 0x1F;

    return (b << 10) | (g << 5) | r;
}

void writeSNESColor(std::vector<uint8_t>& rom, uint32_t addr, const ColorRGBA& c)
{
    uint16_t raw = encodeSNESColor(c);
    rom[addr] = raw & 0xFF;
    rom[addr + 1] = raw >> 8;
}

std::vector<ColorRGBA> decodeNESPalette(const std::vector<uint8_t>& palData)
{
    std::vector<ColorRGBA> out;
    out.reserve(64);

    for(int i = 0; i < 64; ++i)
    {
        uint8_t r = palData[i * 3 + 0];
        uint8_t g = palData[i * 3 + 1];
        uint8_t b = palData[i * 3 + 2];
        out.push_back({ r, g, b, 255 });
    }

    return out;
}

std::array<uint8_t, 4> readNESPaletteIDs(const std::vector<uint8_t>& rom, uint32_t addr)
{
    std::array<uint8_t, 4> ids;
    for(int i = 0; i < 4; ++i)
        ids[i] = rom[addr + i];
    return ids;
}

Palette convertIDsToColors(const std::vector<uint8_t>& ids, const std::vector<ColorRGBA>& master)
{
    Palette out;
    
    int i = 0;
    for (uint8_t id : ids)
    {
        out[i] = master[id];
        ++i;
    }

    return out;
}

std::array<ColorRGBA, 64> decodeNESMasterPalette(const std::vector<uint8_t>& pal)
{
    std::array<ColorRGBA, 64>  out;

    for(int i = 0; i < 64; ++i)
    {
        uint8_t r = pal[i * 3 + 0];
        uint8_t g = pal[i * 3 + 1];
        uint8_t b = pal[i * 3 + 2];
        out[i] = { r, g, b, 255 };
    }

    return out;
}

Palette convertNES4To16(const std::array<uint8_t, 4>& ids, const std::array<ColorRGBA, 64>& master)
{
    Palette out;

    // First 4 NES colors
    const size_t s = master.size();
    for (int i = 0; i < 4; ++i)
    {
        int id = ids[i];
        if (id > s)
            id = 0;
        out[i] = master[id];
    }

    // Fill remaining 12 with black
    for(int i = 4; i < 16; ++i)
        out[i] = {0, 0, 0, 255};

    return out;
}

Palettes makeNESPalettes(const std::vector<uint8_t>& rom, uint32_t addr, const std::array<ColorRGBA, 64>& master, int amount)
{
    Palettes out;

    for (int p = 0; p < amount; ++p)
    {
        auto ids = readNESPaletteIDs(rom, addr + p * 4);
        out[p] = convertNES4To16(ids, master);
    }

    return out;
}

PalAnimation loadAnimatedPalettesNES(std::vector<uint8_t>& rom, uint32_t count_addr, uint32_t timer_addr, uint32_t pal_addr, const std::array<ColorRGBA, 64>& master)
{

    PalAnimation anim;
    anim.frame_count = rom[count_addr];
    anim.frame_timer = rom[timer_addr];
    anim.frames.reserve(anim.frame_count * 4);

    for (int p = 0; p < anim.frame_count; ++p)
    {
        Palettes pals = makeNESPalettes(rom, pal_addr + p * 16, master, 4);
        for(int i = 0; i < 4; ++i)
            anim.frames.push_back(pals[i]);
    }
    return anim;
}

PalAnimation loadAnimatedPalettes(std::vector<uint8_t>& rom, uint32_t count_addr, uint32_t timer_addr, uint32_t pal_addr, const bool hiROM, const Palettes& palettes)
{
    PalAnimation anim;
    anim.frame_count = rom[count_addr];
    anim.frame_timer = rom[timer_addr];

    anim.frames.reserve(anim.frame_count * 6);

    const int blockSize = 32;
    uint32_t ptr = pal_addr;
    uint32_t tableBank = pal_addr & 0xFF0000;

    Palettes current = palettes;

    uint8_t yy = rom[ptr++];

    for (int f = 0; f < anim.frame_count; ++f)
    {
        uint16_t src = rom[ptr] | (rom[ptr + 1] << 8);
        ptr += 2;

        if (src == 0xFFFF)
            continue;

        Palette pal;

        for (int bit = 0; bit < 8; ++bit)
        {
            if (!(yy & (1 << bit)))
                continue;
            
            uint32_t snesAddr = tableBank | (src + bit * blockSize);
            uint32_t srcAddr = snesToPc(snesAddr, hiROM);

            for (int i = 0; i < blockSize; i += 2)
            {
                uint16_t raw = rom[srcAddr + i] | (rom[srcAddr + i + 1] << 8);
                pal[i / 2] = decodeSNESColor(raw);
            }

            current[bit + 2] = pal;
        }
        
        for (int p = 2; p < 8; ++p)
            anim.frames.push_back(current[p]);
    }

    return anim;
}

void writeAnimatedPalettes(std::vector<uint8_t>& rom, uint32_t pal_addr, bool hiROM, const PalAnimation& anim)
{
    const int blockSize = 32;
    uint32_t ptr = pal_addr;
    uint32_t tableBank = pal_addr & 0xFF0000;

    uint8_t yy = 0;
    for (int i = 0; i < 6; i++)
    {
        bool used = false;
        for (const auto& c : anim.frames[i])
        {
            if (c.a != 0 || c.r != 0 || c.g != 0 || c.b != 0)
            {
                used = true;
                break;
            }
        }
        if (used)
            yy |= (1 << i);
    }

    rom[ptr] = yy;

    ++ptr;

    Palettes current;
    for (int i = 0; i < 6; i++)
        current[i] = anim.frames[i];

    int frameIndex = 0;

    for (int f = 0; f < anim.frame_count; f++)
    {
        uint16_t src = rom[ptr] | (rom[ptr + 1] << 8);
        ptr += 2;

        if (src == 0xFFFF)
        {
            frameIndex += 6;
            continue;
        }

        for (int bit = 0; bit < 6; bit++)
        {
            if (!(yy & (1 << bit)))
                continue;

            const Palette& pal = anim.frames[frameIndex + bit];

            uint32_t snesAddr = tableBank | (src + bit * blockSize);
            uint32_t dstAddr = snesToPc(snesAddr, hiROM);

            for (int i = 0; i < blockSize; i += 2)
            {
                writeSNESColor(rom, dstAddr + i, pal[i / 2]);
            }

            current[bit] = pal;
        }

        frameIndex += 6;
    }
}