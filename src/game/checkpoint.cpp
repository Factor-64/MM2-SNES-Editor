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

/*void saveCheckpoints(std::vector<uint8_t>& rom, uint32_t addr, const Checkpoints& c)
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
}*/

DataChanged saveCheckpoint(std::vector<uint8_t>& rom, uint32_t addr, const Checkpoint& c)
{
    DataChanged d;
    MemoryDelta mem;
    mem.address = addr;
    mem.newData.push_back(c.y);
    mem.oldData.push_back(rom[addr]);
    d.deltas.push_back(mem);
    mem.newData.clear();
    mem.oldData.clear();
    addr += 0x06;

    mem.address = addr;
    mem.newData.push_back(c.screen);
    mem.oldData.push_back(rom[addr]);
    d.deltas.push_back(mem);
    mem.newData.clear();
    mem.oldData.clear();
    addr += 0x06;

    mem.address = addr;
    mem.newData.push_back(c.enemy_index);
    mem.oldData.push_back(rom[addr]);
    d.deltas.push_back(mem);
    mem.newData.clear();
    mem.oldData.clear();
    addr += 0x06;

    mem.address = addr;
    mem.newData.push_back(c.item_index);
    mem.oldData.push_back(rom[addr]);
    d.deltas.push_back(mem);
    mem.newData.clear();
    mem.oldData.clear();
    addr += 0x06;

    mem.address = addr;
    mem.newData.push_back((c.map_back_addr >> 8) & 0xFF);
    mem.oldData.push_back(rom[addr]);
    d.deltas.push_back(mem);
    mem.newData.clear();
    mem.oldData.clear();
    addr += 0x06;

    mem.address = addr;
    mem.newData.push_back(c.map_back_addr & 0xFF);
    mem.oldData.push_back(rom[addr]);
    d.deltas.push_back(mem);
    mem.newData.clear();
    mem.oldData.clear();
    addr += 0x06;

    mem.address = addr;
    mem.newData.push_back((c.map_forward_addr >> 8) & 0xFF);
    mem.oldData.push_back(rom[addr]);
    d.deltas.push_back(mem);
    mem.newData.clear();
    mem.oldData.clear();
    addr += 0x06;

    mem.address = addr;
    mem.newData.push_back(c.map_forward_addr & 0xFF);
    mem.oldData.push_back(rom[addr]);
    d.deltas.push_back(mem);
    mem.newData.clear();
    mem.oldData.clear();
    addr += 0x06;

    mem.address = addr;
    mem.newData.push_back(c.scroll);
    mem.oldData.push_back(rom[addr]);
    d.deltas.push_back(mem);
    mem.newData.clear();
    mem.oldData.clear();
    addr += 0x06;

    mem.address = addr;
    mem.newData.push_back(c.left_screen);
    mem.oldData.push_back(rom[addr]);
    d.deltas.push_back(mem);
    mem.newData.clear();
    mem.oldData.clear();
    addr += 0x06;

    mem.address = addr;
    mem.newData.push_back(c.right_screen);
    mem.oldData.push_back(rom[addr]);
    d.deltas.push_back(mem);
    return d;
}

std::array<BGPositionData, 3> loadBGPositionData(const std::vector<uint8_t>& rom, uint32_t addr)
{
    std::array<BGPositionData, 3> data;
    for (int i = 0; i < data.size(); ++i)
    {
        BGPositionData out;
        addr += i * 10;
        out.scrollId = rom[addr + 0];
        out.bg2_x = rom[addr + 1] | (rom[addr + 2] << 8);
        out.bg2_y = rom[addr + 3] | (rom[addr + 4] << 8);
        out.bg3_x = rom[addr + 5] | (rom[addr + 6] << 8);
        out.bg3_y = rom[addr + 7] | (rom[addr + 8] << 8);
        out.bg2_screenId = rom[addr + 9];
        data[i] = out;
    }
    return data;
}

/*void saveBGPositionData(std::vector<uint8_t>& rom, uint32_t addr, const BGPositionData& data)
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
}*/

MemoryDelta saveBGPositionData(std::vector<uint8_t>& rom, uint32_t addr, const BGPositionData& data)
{
    MemoryDelta m;
    m.address = addr;
    m.newData.push_back(data.scrollId);
    m.oldData.push_back(rom[addr]);
    
    m.newData.push_back(data.bg2_x & 0xFF);
    m.oldData.push_back(rom[addr + 1]);

    m.newData.push_back((data.bg2_x >> 8) & 0xFF);
    m.oldData.push_back(rom[addr + 2]);

    m.newData.push_back(data.bg2_y & 0xFF);
    m.oldData.push_back(rom[addr + 3]);

    m.newData.push_back((data.bg2_y >> 8) & 0xFF);
    m.oldData.push_back(rom[addr + 4]);

    m.newData.push_back(data.bg3_x & 0xFF);
    m.oldData.push_back(rom[addr + 5]);

    m.newData.push_back((data.bg3_x >> 8) & 0xFF);
    m.oldData.push_back(rom[addr + 6]);

    m.newData.push_back(data.bg3_y & 0xFF);
    m.oldData.push_back(rom[addr + 7]);

    m.newData.push_back((data.bg3_y >> 8) & 0xFF);
    m.oldData.push_back(rom[addr + 8]);

    m.newData.push_back(data.bg2_screenId);
    m.oldData.push_back(rom[addr + 9]);

    return m;
}