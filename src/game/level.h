#pragma once
#include <vector>
#include <array>

struct ScrollEnable {
    bool bg2 = false;
    bool bg3 = false;
};



struct BGTilemapMirror {
    uint8_t bg2_mode = 0x28; // 28, 29, 2A
    uint8_t bg3_mode = 0x30; // 30, 31, 32
    /*
    28 = 32x32 single screen mirroring
    29 = 64x32 vertical mirroring (horizontal scroll)
	2A = 32x64 horizontal mirroring (vertical scroll)
    30 = 32x32 single screen mirroring
    31 = 64x32 vertical mirroring (horizontal scroll)
    32 = 32x64 horizontal mirroring (vertical scroll)
    */
};

struct BGSpeedData
{
    uint8_t frames = 0;
    uint8_t scanlines = 0;
    uint8_t frame_count = 0;
};

std::vector<uint8_t> loadLevelData(const std::vector<uint8_t>& rom, uint32_t addr, int count);
std::vector<uint8_t> loadScrollData(const std::vector<uint8_t>& rom, uint32_t addr, int count);
std::vector<uint8_t> remapColumnMajorScreensHorizontally(const std::vector<uint8_t>& levelData);
int metaWidthFromLevelData(const std::vector<uint8_t>& levelData);

void saveLevelData(std::vector<uint8_t>& rom, uint32_t addr, const std::vector<uint8_t>& newData);
void saveScrollData(std::vector<uint8_t>& rom, uint32_t addr, const std::vector<uint8_t>& data);

std::array<ScrollEnable, 64> loadBackgroundScrollData(const std::vector<uint8_t>& rom, uint32_t addr);
void saveBackgroundScrollData(std::vector<uint8_t>& rom, uint32_t addr, const std::array<ScrollEnable, 64>& data);



std::array<BGTilemapMirror, 64> loadBGTilemapMirror(const std::vector<uint8_t>& rom, uint32_t addr);
void saveBGTilemapMirror(std::vector<uint8_t>& rom, uint32_t addr, const std::array<BGTilemapMirror, 64>& data);

std::array<std::array<BGSpeedData, 4>, 32> loadBGScrollSpeeds(const std::vector<uint8_t>& rom, uint32_t addr);
void saveBGScrollSpeeds(std::vector<uint8_t>& rom, uint32_t addr, const std::array<std::array<BGSpeedData, 4>, 32>& data);
