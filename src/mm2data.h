#pragma once
#include <string>
#include <map>
#include <vector>
#include <cstdint>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct Range {
    uint32_t start;
    uint32_t end;
};

struct GFXData {
    std::vector<Range> ranges;
    std::vector<int> commonIdx;
};

struct LevelEntry {
    // metatile
    uint32_t chip32x32;

    // metatile palette
    uint32_t chip32x32_palette;

    // level data
    uint32_t map;

    // level scroll data
    uint32_t scroll;

    // enemy
    uint32_t enemy_screen;
    uint32_t enemy_x;
    uint32_t enemy_y;
    uint32_t enemy_type;

    // item
    uint32_t item_screen;
    uint32_t item_x;
    uint32_t item_y;
    uint32_t item_type;

    // midpoint
    uint32_t midpoint_start_y;
    uint32_t midpoint_start_room;
    uint32_t midpoint_enemy_index;
    uint32_t midpoint_item_index;
    uint32_t midpoint_map_back_high;
    uint32_t midpoint_map_back_low;
    uint32_t midpoint_map_forward_high;
    uint32_t midpoint_map_forward_low;
    uint32_t midpoint_start_scroll;
    uint32_t midpoint_left_room;
    uint32_t midpoint_right_room;

    // pattern table
    uint32_t pattern;

    // palette data
    uint32_t palette_data;

    // palette animation data
    uint32_t palette_afc;
    uint32_t palette_aft;
    uint32_t palette_anime;
};

struct MM2_Data {
    std::map<std::string, GFXData> gfx;
    std::map<std::string, LevelEntry> levels;

    std::vector<std::string> levelNames;

    std::string modeName;
};

enum LevelField {
    LF_CommonGFX,
    LF_LevelGFX,
    LF_MetaTileData,
    LF_MetaTilePaletteData,
    LF_LevelData,
    LF_ScrollData,
    LF_EnemyData,
    LF_ItemData,
    LF_MidPointData,
    LF_PatternTable,
    LF_PaletteData,
    LF_PaletteAnimationData,
    LF_END
};

std::vector<MM2_Data> loadMM2Data(const std::string& path, const bool hiROM);
