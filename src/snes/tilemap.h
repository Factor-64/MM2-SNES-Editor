#pragma once
#include <vector>
#include <glad/glad.h>
#include "tiledecoder.h"

struct TileMap {
    int width = 0;
    int height = 0;
    uint8_t shape = 0; // shape = 1 8x16, shape = 2 16x16
    std::vector<TileRef> cells; // width * height entries

    TileRef& at(int x, int y) {
        return cells[y * width + x];
    }

    const TileRef& at(int x, int y) const {
        return cells[y * width + x];
    }
};

struct TilemapTexture {
    GLuint tex = 0;
    int width = 0;
    int height = 0;
};

TileMap makeTileMap(const std::vector<Tile>& atlas, int mapWidth, uint8_t shape);

void renderTileMapToRGBA(const TileMap& map,const std::vector<Tile>& atlas, const Palette& palette, 
    const ColorRGBA& bgColor, std::vector<ColorRGBA>& outPixels, int& width, int& height);

void uploadTilemapTextureRGBA(const std::vector<ColorRGBA>& pixels, TilemapTexture& t);

std::vector<MacroTile> buildMacroTiles(const TileMap& map);

void renderMetaTileMapToRGBA(const std::vector<MetaTile>& metaTiles, int metaWidth, const std::vector<Tile>& atlas, 
    const Palettes& palettes, const uint8_t palOffset, const ColorRGBA& bgColor, std::vector<ColorRGBA>& outPixels, int& width, int& height);

void renderMetaTileWindowToRGBA(
    const std::vector<uint8_t>& metaIndices,
    int fullMetaWidth,
    int windowX,              // starting meta-tile X
    int windowWidth,          // number of meta-tiles to draw
    const std::vector<MetaTile>& metaTiles,
    const std::vector<Tile>& atlas,
    const Palettes& palettes,
    const uint8_t palOffset,
    const ColorRGBA& bgColor,
    std::vector<ColorRGBA>& outPixels,
    int& outW,
    int& outH);

void renderBGTileMapToRGBA(
    const std::vector<BGTileData>& bgTileData,
    int screenWidth, // in tiles
    const std::vector<Tile>& atlas,
    const Palettes& palettes,
    const ColorRGBA& bgColor,
    std::vector<ColorRGBA>& outPixels,
    int& outW,
    int& outH);
