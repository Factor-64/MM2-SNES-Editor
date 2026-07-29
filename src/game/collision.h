#pragma once
#include <../../imgui/imgui.h>
#include <cstdint>
struct CollisionType {
    const char* name;
    ImU32 color;
    uint8_t id;
};

static CollisionType collisionTypes[] = {
    { "None",         IM_COL32(0,0,0,120),   0 },
    { "Solid",        IM_COL32(0,0,255,120), 1 },
    { "Ladder/Water", IM_COL32(0,255,0,120), 2 },
    { "Instant Kill", IM_COL32(255,0,0,120), 3 }
};