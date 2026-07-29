#pragma once
#include "cgramdecoder.h"
#include <glad/glad.h>
#include <array>
#include "../mm2data.h"

struct Tile {
    std::array<uint8_t, 64> pixels;
    uint32_t romAddress;
};

struct MetaTileData {
    std::array<uint16_t, 4> tileIndexes;
    std::array<uint8_t, 4>  palettes;
    std::array<uint8_t, 4>  collision;
};

std::vector<Tile> decodeTileRanges(const std::vector<Range>& ranges, const std::vector<uint8_t>& rom);

std::vector<MetaTileData> decodeMetaTile32(const std::vector<uint8_t>& rom, uint32_t addr, int count);
void loadMetaTilePalettes(std::vector<MetaTileData>& data, const std::vector<uint8_t>& rom, uint32_t addr, int count);
void encodeMetaTile32(std::vector<uint8_t>& rom, uint32_t addr, const std::vector<MetaTileData>& metaTiles);
void saveMetaTilePalettes(std::vector<uint8_t>& rom, uint32_t addr, const std::vector<MetaTileData>& data);
void saveTileToROM(Tile& t, std::vector<uint8_t>& rom);
