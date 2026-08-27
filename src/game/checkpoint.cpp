#include "checkpoint.h"

Checkpoints loadCheckpoints(std::vector<uint8_t>& rom, uint32_t addr)
{
    Checkpoints c;

    for (int i = 0; i < c.size(); ++i)
    {
        c[i].y = rom[addr + i];
        c[i].screen = rom[addr + 0x06 + i];
        c[i].enemy_index = rom[addr + 0x0C + i];
        c[i].item_index = rom[addr + 0x12 + i];

        uint8_t back_hi = rom[addr + 0x18 + i];
        uint8_t back_lo = rom[addr + 0x1E + i];
        c[i].map_back_addr = (back_hi << 8) | back_lo;

        uint8_t fwd_hi = rom[addr + 0x24 + i];
        uint8_t fwd_lo = rom[addr + 0x2A + i];
        c[i].map_forward_addr = (fwd_hi << 8) | fwd_lo;

        c[i].scroll = rom[addr + 0x30 + i];
        c[i].left_screen = rom[addr + 0x36 + i];
        c[i].right_screen = rom[addr + 0x3C + i];
    }

    return c;
}

void saveCheckpoints(std::vector<uint8_t>& rom, uint32_t addr, const Checkpoints& c)
{
    for (int i = 0; i < 3; ++i)
    {
        rom[addr + i] = c[i].y;
        rom[addr + 0x06 + i] = c[i].screen;
        rom[addr + 0x0C + i] = c[i].enemy_index;
        rom[addr + 0x12 + i] = c[i].item_index;

        rom[addr + 0x18 + i] = (c[i].map_back_addr >> 8) & 0xFF;  // high byte
        rom[addr + 0x1E + i] = c[i].map_back_addr & 0xFF;  // low byte

        rom[addr + 0x24 + i] = (c[i].map_forward_addr >> 8) & 0xFF;
        rom[addr + 0x2A + i] = c[i].map_forward_addr & 0xFF;

        rom[addr + 0x30 + i] = c[i].scroll;
        rom[addr + 0x36 + i] = c[i].left_screen;
        rom[addr + 0x3C + i] = c[i].right_screen;
    }
}

BGPositionData loadBGPositionData(const std::vector<uint8_t>& rom, uint32_t addr)
{
    BGPositionData out;

    out.scrollId = rom[addr + 0];
    out.bg2_x = rom[addr + 1] | (rom[addr + 2] << 8);
    out.bg2_y = rom[addr + 3] | (rom[addr + 4] << 8);
    out.bg3_x = rom[addr + 5] | (rom[addr + 6] << 8);
    out.bg3_y = rom[addr + 7] | (rom[addr + 8] << 8);
    out.bg2_screenId = rom[addr + 9];

    return out;
}

void saveBGPositionData(std::vector<uint8_t>& rom, uint32_t addr, const BGPositionData& data)
{
    rom[addr + 0] = data.scrollId;

    rom[addr + 1] = data.bg2_x & 0xFF;
    rom[addr + 2] = (data.bg2_x >> 8) & 0xFF;

    rom[addr + 3] = data.bg2_y & 0xFF;
    rom[addr + 4] = (data.bg2_y >> 8) & 0xFF;

    rom[addr + 5] = data.bg3_x & 0xFF;
    rom[addr + 6] = (data.bg3_x >> 8) & 0xFF;

    rom[addr + 7] = data.bg3_y & 0xFF;
    rom[addr + 8] = (data.bg3_y >> 8) & 0xFF;

    rom[addr + 9] = data.bg2_screenId;
}
