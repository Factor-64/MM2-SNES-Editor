#include "level.h"
#include <print>

std::vector<uint8_t> loadLevelData(const std::vector<uint8_t>& rom, uint32_t addr, int count)
{
	return std::vector<uint8_t>(rom.begin() + addr, rom.begin() + addr + count);
}

ScrollData loadScrollData(const std::vector<uint8_t>& rom, uint32_t addr)
{
    ScrollData buf;
    std::copy(rom.begin() + addr, rom.begin() + addr + buf.size(), buf.begin());
    return buf;
}

std::vector<uint8_t> remapColumnMajorScreensHorizontally(const std::vector<uint8_t>& levelData)
{
    const int screenW = 8;
    const int screenH = 8;
    const int tilesPerScreen = screenW * screenH;

    int numScreens = levelData.size() / tilesPerScreen;

    std::vector<uint8_t> out;
    out.resize(levelData.size());

    for (int s = 0; s < numScreens; ++s)
    {
        const int screenOffset = s * tilesPerScreen;

        for (int col = 0; col < screenW; ++col)
        {
            for (int row = 0; row < screenH; ++row)
            {
                int src = screenOffset + col * screenH + row;
                int dstRow = row;
                int dstCol = s * screenW + col;

                int dst = dstRow * (numScreens * screenW) + dstCol;

                out[dst] = levelData[src];
            }
        }
    }

    return out;
}

std::vector<uint8_t> remapBackToColumnMajor(const std::vector<uint8_t>& data)
{
    const int screenW = 8;
    const int screenH = 8;
    const int tilesPerScreen = screenW * screenH;

    int numScreens = data.size() / tilesPerScreen;

    std::vector<uint8_t> out(data.size());

    for (int s = 0; s < numScreens; ++s)
    {
        const int screenOffset = s * tilesPerScreen;

        for (int col = 0; col < screenW; ++col)
        {
            for (int row = 0; row < screenH; ++row)
            {
                int srcRow = row;
                int srcCol = s * screenW + col;

                int src = srcRow * (numScreens * screenW) + srcCol;

                int dst = screenOffset + col * screenH + row;

                out[dst] = data[src];
            }
        }
    }

    return out;
}

MemoryDelta saveLevelData(std::vector<uint8_t>& rom, uint32_t addr, int screenIndex, int row, int col, uint8_t value)
{
    MemoryDelta m;
    const int screenW = 8;
    const int screenH = 8;
    const int tilesPerScreen = screenW * screenH;

    int tileIndexColumnMajor = col * screenH + row;
    uint32_t romOffset = addr + screenIndex * tilesPerScreen + tileIndexColumnMajor;
    m.address = romOffset;
    m.newData.push_back(value);
    m.oldData.push_back(rom[romOffset]);
    return m;
}

int metaWidthFromLevelData(const std::vector<uint8_t>& levelData)
{
    const int screenSize = 8 * 8;
    int numScreens = levelData.size() / screenSize;
    return numScreens * 8;
}

std::array<ScrollEnable, 64> loadBackgroundScrollData(const std::vector<uint8_t>& rom, uint32_t addr)
{
    std::array<ScrollEnable, 64> out;

    for (size_t i = 0; i < out.size(); ++i) 
    {
        uint8_t b = rom[addr + i];
        out[i].bg2 = (b & 0x10) != 0;
        out[i].bg3 = (b & 0x01) != 0;
    }
    return out;
}

std::array<BGTilemapMirror, 64> loadBGTilemapMirror(const std::vector<uint8_t>& rom, uint32_t addr)
{
    std::array<BGTilemapMirror, 64> out;

    for (size_t i = 0; i < out.size(); ++i) 
    {
        out[i].bg2_mode = rom[addr + i * 2 + 0];
        out[i].bg3_mode = rom[addr + i * 2 + 1];
    }
    return out;
}

std::array<std::array<BGSpeedData, 4>, 32> loadBGScrollSpeeds(const std::vector<uint8_t>& rom, uint32_t addr)
{
    std::array<std::array<BGSpeedData, 4>, 32> out;

    for (size_t i = 0; i < out.size(); ++i)
    {
        uint32_t base = addr + i * 4;

        for (size_t j = 0; j < 4; ++j)
        {
            uint8_t b = rom[base + j];

            out[i][j].frames = b >> 4;
            out[i][j].scanlines = b & 0x0F;
            out[i][j].frame_count = b >> 4;
        }
    }

    return out;
}

MemoryDelta saveBackgroundScrollData(std::vector<uint8_t>& rom, uint32_t addr, const ScrollEnable& data)
{
    uint8_t b = 0;
    if (data.bg2) b |= 0x10;
    if (data.bg3) b |= 0x01;

    MemoryDelta mem;
    mem.address = addr;
    mem.newData.push_back(b);
    mem.oldData.push_back(rom[addr]);
    return mem;
}

MemoryDelta saveBGTilemapMirror(std::vector<uint8_t>& rom, uint32_t addr, const BGTilemapMirror& data)
{
    MemoryDelta mem;
    mem.address = addr;
    mem.newData.push_back(data.bg2_mode);
    mem.newData.push_back(data.bg3_mode);
    mem.oldData.push_back(rom[addr]);
    mem.oldData.push_back(rom[addr + 1]);
    return mem;
}

MemoryDelta saveBGScrollSpeeds(std::vector<uint8_t>& rom, uint32_t addr, const std::array<BGSpeedData, 4>& data)
{
    MemoryDelta mem;
    mem.address = addr;
    for (int i = 0; i < data.size(); ++i)
    {
        uint8_t b = (data[i].frames << 4) | (data[i].scanlines & 0x0F);
        mem.newData.push_back(b);
        mem.oldData.push_back(rom[addr]);
    }
    return mem;
}
