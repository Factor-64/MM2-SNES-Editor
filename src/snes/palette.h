#pragma once
#include <array>
#include <vector>
struct ColorRGBA {
    uint8_t r = 0, g = 0, b = 0, a = 0;
};

using Palette = std::array<ColorRGBA, 16>;
using Palettes = std::array<Palette, 16>;
