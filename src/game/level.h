#pragma once
#include <vector>

std::vector<uint8_t> loadLevelData(const std::vector<uint8_t>& rom, uint32_t addr, int count);
std::vector<uint8_t> loadScrollData(const std::vector<uint8_t>& rom, uint32_t addr, int count);
std::vector<uint8_t> remapColumnMajorScreensHorizontally(const std::vector<uint8_t>& levelData);
int metaWidthFromLevelData(const std::vector<uint8_t>& levelData);

void saveLevelData(std::vector<uint8_t>& rom, uint32_t addr, const std::vector<uint8_t>& newData);
void saveScrollData(std::vector<uint8_t>& rom, uint32_t addr, const std::vector<uint8_t>& data);