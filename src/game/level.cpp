#include "level.h"
#include <print>

std::vector<uint8_t> loadLevelData(const std::vector<uint8_t>& rom, uint32_t addr, int count)
{
	return std::vector<uint8_t>(rom.begin() + addr, rom.begin() + addr + count);
}

std::vector<uint8_t> loadScrollData(const std::vector<uint8_t>& rom, uint32_t addr, int count)
{
    return std::vector<uint8_t>(rom.begin() + addr, rom.begin() + addr + count);
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

void saveLevelData(std::vector<uint8_t>& rom, uint32_t addr, const std::vector<uint8_t>& newData)
{
    auto data = remapBackToColumnMajor(newData);
    std::copy(data.begin(), data.end(), rom.begin() + addr);
}

void saveScrollData(std::vector<uint8_t>& rom, uint32_t addr, const std::vector<uint8_t>& data)
{
    std::copy(data.begin(), data.end(), rom.begin() + addr);
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

BGPositionData loadBGPositionData(const std::vector<uint8_t>& rom, uint32_t addr)
{
    BGPositionData out;

    out.scrollId = rom[addr + 0];
    out.bg2_x = rom[addr + 1] | (rom[addr + 2] << 8);
    out.bg2_y = rom[addr + 3] | (rom[addr + 4] << 8);
    out.bg3_x = rom[addr + 5] | (rom[addr + 6] << 8);
    out.bg3_y = rom[addr + 7] | (rom[addr + 8] << 8);
    out.bg2_screenId = rom[addr + 9];

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

void saveBackgroundScrollData(std::vector<uint8_t>& rom, uint32_t addr, const std::array<ScrollEnable, 64>& data)
{
    for (size_t i = 0; i < data.size(); ++i)
    {
        uint8_t b = 0;
        if (data[i].bg2) b |= 0x10;
        if (data[i].bg3) b |= 0x01;

        rom[addr + i] = b;
    }
}

void saveBGPositionData(std::vector<uint8_t>& rom, uint32_t addr, const BGPositionData& data)
{
    rom[addr + 0] = data.scrollId;

    rom[addr + 1] = data.bg2_x & 0xFF;
    rom[addr + 2] = (data.bg2_x >> 8) & 0xFF;

    rom[addr + 3] = data.bg2_y & 0xFF;
    rom[addr + 4] = (data.bg2_y >> 8) & 0xFF;

    rom[addr + 5] = data.bg3_x & 0xFF;
    rom[addr + 6] = (data.bg3_x >> 8) & 0xFF;

    rom[addr + 7] = data.bg3_y & 0xFF;
    rom[addr + 8] = (data.bg3_y >> 8) & 0xFF;

    rom[addr + 9] = data.bg2_screenId;
}

void saveBGTilemapMirror(std::vector<uint8_t>& rom, uint32_t addr, const std::array<BGTilemapMirror, 64>& data)
{
    for (size_t i = 0; i < data.size(); ++i)
    {
        rom[addr + i * 2 + 0] = data[i].bg2_mode;
        rom[addr + i * 2 + 1] = data[i].bg3_mode;
    }
}

void saveBGScrollSpeeds(std::vector<uint8_t>& rom, uint32_t addr, const std::array<std::array<BGSpeedData, 4>, 32>& data)
{
    for (size_t i = 0; i < data.size(); ++i)
    {
        uint32_t base = addr + i * 4;

        for (size_t j = 0; j < 4; ++j)
        {
            const BGSpeedData& s = data[i][j];

            uint8_t b = (s.frames << 4) | (s.scanlines & 0x0F);
            rom[base + j] = b;
        }
    }
}
