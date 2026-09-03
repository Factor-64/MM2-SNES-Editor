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

struct TileRef {
    uint32_t index = 0;   // index of the TOP-LEFT 8x8 tile
};

struct MetaTileData {
    std::array<uint8_t, 4> tileIndexes;
    std::array<uint8_t, 4> palettes;
    std::array<uint8_t, 4> collision;
};

struct MacroTile {
    TileRef left;
    TileRef right;
};

struct MetaTile {
    MacroTile tiles[4];
    uint8_t macroIndex[4];
    std::array<uint8_t, 4> palettes;
    std::array<uint8_t, 4> collision;
};

std::vector<MetaTile> makeMetaTiles(const std::vector<MetaTileData>& data, const std::vector<MacroTile>& macroTiles);
DataChanged saveMetaTileToROM(std::vector<uint8_t>& rom, uint32_t addr, uint32_t paladdr, uint32_t collision, MetaTile& mt);
std::vector<Tile> decodeTileRanges(const std::vector<Range>& ranges, const std::vector<uint8_t>& rom, const int tileSize);
std::vector<Tile> decodeTileRange(const Range& range, const std::vector<uint8_t>& rom, const int tileSize);

std::vector<MetaTileData> decodeMetaTile32NES(const std::vector<uint8_t>& rom, uint32_t addr, int count);
std::vector<MetaTileData> decodeMetaTile32SNES(const std::vector<uint8_t>& rom, uint32_t addr, uint32_t collision, int count);
void loadMetaTilePalettes(std::vector<MetaTileData>& data, const std::vector<uint8_t>& rom, uint32_t addr, int count);

MemoryDelta saveTileToROM(Tile& t, std::vector<uint8_t>& rom, const bool is2bpp = false);
std::vector<BGTileData> loadBackgroundTileData(std::vector<uint8_t>& rom, uint32_t addr, int count);
MemoryDelta saveBackgroundTileData(std::vector<uint8_t>& rom, uint32_t addr, const BGTileData& td);
