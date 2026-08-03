#pragma once
#include <vector>
#include <cstdint>
#include "palette.h"

struct PalAnimation {
    uint8_t frame_count = 0;
    int maxFrameCount = 0;
    uint8_t frame_timer = 0;
    uint8_t palette_bits = 0;
    std::vector<Palette> frames;
};

Palettes makeNESPalettes(const std::vector<uint8_t>& rom, uint32_t addr, const std::array<ColorRGBA, 64>& master, int amount = 8);
std::array<ColorRGBA, 64> decodeNESMasterPalette(const std::vector<uint8_t>& pal);
Palettes decodeCGRAMPalettes(const std::vector<uint8_t>& rom, uint32_t addr, int paletteCount);
void writeSNESColor(std::vector<uint8_t>& rom, uint32_t addr, const ColorRGBA& c);
PalAnimation loadAnimatedPalettesNES(std::vector<uint8_t>& rom, uint32_t count_addr, uint32_t timer_addr, uint32_t pal_addr, const std::array<ColorRGBA, 64>& master);
PalAnimation loadAnimatedPalettes(std::vector<uint8_t>& rom, uint32_t count_addr, uint32_t timer_addr, uint32_t pal_addr, const bool hiROM, const Palettes& palettes);
void writeAnimatedPalettes(std::vector<uint8_t>& rom, uint32_t pal_addr, bool hiROM, const PalAnimation& anim);