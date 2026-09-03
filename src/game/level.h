#pragma once
#include <vector>
#include <array>
#include "../undoredo.h"

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

struct BGSpeedData {
    uint8_t frames = 0;
    uint8_t scanlines = 0;
    uint8_t frame_count = 0;
};

using ScrollData = std::array<uint8_t, 512>;

std::vector<uint8_t> loadLevelData(const std::vector<uint8_t>& rom, uint32_t addr, int count);
ScrollData loadScrollData(const std::vector<uint8_t>& rom, uint32_t addr);
std::vector<uint8_t> remapColumnMajorScreensHorizontally(const std::vector<uint8_t>& levelData);
int metaWidthFromLevelData(const std::vector<uint8_t>& levelData);

MemoryDelta saveLevelData(std::vector<uint8_t>& rom, uint32_t addr, int screenIndex, int row, int col, uint8_t value);

std::array<ScrollEnable, 64> loadBackgroundScrollData(const std::vector<uint8_t>& rom, uint32_t addr);
MemoryDelta saveBackgroundScrollData(std::vector<uint8_t>& rom, uint32_t addr, const ScrollEnable& data);

std::array<BGTilemapMirror, 64> loadBGTilemapMirror(const std::vector<uint8_t>& rom, uint32_t addr);
MemoryDelta saveBGTilemapMirror(std::vector<uint8_t>& rom, uint32_t addr, const BGTilemapMirror& data);

std::array<std::array<BGSpeedData, 4>, 32> loadBGScrollSpeeds(const std::vector<uint8_t>& rom, uint32_t addr);
MemoryDelta saveBGScrollSpeeds(std::vector<uint8_t>& rom, uint32_t addr, const std::array<BGSpeedData, 4>& data);
