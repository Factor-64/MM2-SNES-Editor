#pragma once
#include "cgramdecoder.h"
#include <glad/glad.h>
#include <array>
#include "../mm2data.h"

struct Tile {
    std::array<uint8_t, 64> pixels;
    uint32_t romAddress = 0;
};

struct BGTileData {
    uint8_t tileId = 0;
    uint8_t vramPage = 0;
    uint8_t palette = 0;
    bool highPriority = false;
    bool hFlip = false;
    bool vFlip = false;
};

struct MetaTileData {
    std::array<uint16_t, 4> tileIndexes;
    std::array<uint8_t, 4>  palettes;
    std::array<uint8_t, 4>  collision;
};

std::vector<Tile> decodeTileRanges(const std::vector<Range>& ranges, const std::vector<uint8_t>& rom, const int tileSize = 32);

std::vector<MetaTileData> decodeMetaTile32NES(const std::vector<uint8_t>& rom, uint32_t addr, int count);
std::vector<MetaTileData> decodeMetaTile32SNES(const std::vector<uint8_t>& rom, uint32_t addr, uint32_t collision, int count);
void loadMetaTilePalettes(std::vector<MetaTileData>& data, const std::vector<uint8_t>& rom, uint32_t addr, int count);
void encodeMetaTile32NES(std::vector<uint8_t>& rom, uint32_t addr, const std::vector<MetaTileData>& metaTiles);
void encodeMetaTile32SNES(std::vector<uint8_t>& rom, uint32_t addr, uint32_t collision, const std::vector<MetaTileData>& metaTiles);
void saveMetaTilePalettes(std::vector<uint8_t>& rom, uint32_t addr, const std::vector<MetaTileData>& data);
void saveTileToROM(Tile& t, std::vector<uint8_t>& rom);
std::vector<BGTileData> loadBackgroundTileData(std::vector<uint8_t>& rom, uint32_t addr, int count);
void saveBackgroundTileData(std::vector<uint8_t>& rom, uint32_t addr, std::vector<BGTileData> data);
