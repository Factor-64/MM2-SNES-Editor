#pragma once
#include "snes/palette.h"
#include "snes/tiledecoder.h"
#include <string>
#include <vector>

// std::vector<uint8_t>& pixels should be in scanline format
bool loadIndexedPNG(const std::string& filename, Palettes& palettes, std::vector<uint8_t>& pixels, int& width, int& height);
bool loadIndexedBMP(const std::string& filename, Palettes& palettes, std::vector<uint8_t>& pixels, int& width, int& height);
std::vector<Tile> extractTiles(const std::vector<uint8_t>& pixels, int width, int height);
std::vector<Tile> convert4bppTo2bpp(const std::vector<Tile>& tiles);