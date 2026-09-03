#pragma once
#include <array>
#include <vector>
#include "../undoredo.h"

struct Checkpoint {
    uint8_t y = 0;
    uint8_t screen = 0;
    uint8_t enemy_index = 0;
    uint8_t item_index = 0;
    uint16_t map_back_addr = 0;
    uint16_t map_forward_addr = 0;
    uint8_t scroll = 0;
    uint8_t left_screen = 0;
    uint8_t right_screen = 0;
};

struct BGPositionData {
    uint8_t scrollId = 0;
    uint16_t bg2_x = 0;
    uint16_t bg2_y = 0;
    uint16_t bg3_x = 0;
    uint16_t bg3_y = 0;
    uint8_t bg2_screenId = 0;
};

using Checkpoints = std::array<Checkpoint, 3>;

Checkpoints loadCheckpoints(std::vector<uint8_t>& rom, uint32_t addr);
DataChanged saveCheckpoint(std::vector<uint8_t>& rom, uint32_t addr, const Checkpoint& c);

std::array<BGPositionData, 3> loadBGPositionData(const std::vector<uint8_t>& rom, uint32_t addr);
MemoryDelta saveBGPositionData(std::vector<uint8_t>& rom, uint32_t addr, const BGPositionData& data);