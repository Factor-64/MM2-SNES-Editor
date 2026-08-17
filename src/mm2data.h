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
    std::vector<Range> layer12;
    std::vector<Range> layer3;
    std::vector<int> commonIdx;
};

struct LevelEntry {
    // metatile
    uint32_t chip32x32;

    // metatile palette
    uint32_t chip32x32_palette;

    // level data
    uint32_t map;
    uint32_t collision;

    // level scroll data
    uint32_t scroll;

    //bg data
    uint32_t bg_tilemap;
    uint32_t bg_scroll;
    uint32_t bg_start;
    uint32_t bg_checkpoint;
    uint32_t bg_boss;
    uint32_t bg_mirror;
    uint32_t bg_speed;

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
    /*uint32_t midpoint_start_room;
    uint32_t midpoint_enemy_index;
    uint32_t midpoint_item_index;
    uint32_t midpoint_map_back_high;
    uint32_t midpoint_map_back_low;
    uint32_t midpoint_map_forward_high;
    uint32_t midpoint_map_forward_low;
    uint32_t midpoint_start_scroll;
    uint32_t midpoint_left_room;
    uint32_t midpoint_right_room;*/

    // pattern table
    uint32_t pattern;

    // palette data
    uint32_t palette_data;

    // palette animation data
    uint32_t palette_afc;
    uint32_t palette_aft;
    uint32_t palette_anime;

    // layer 2 & 3 palette
    uint32_t palette_layer2;
    uint32_t palette_layer3;
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
    LF_CollisionData,
    LF_ScrollData,
    LF_Layer2TilemapData,
    LF_Layer3TilemapData,
    LF_BGScrollSpeedData,
    LF_BGTilemapMirroring,
    LF_BGScrollEnable,
    LF_EnemyData,
    LF_ItemData,
    LF_MidPointData,
    LF_PatternTable,
    LF_PaletteData,
    LF_Layer2PaletteData,
    LF_Layer3PaletteData,
    LF_PaletteAnimationData,
    LF_END
};

std::vector<MM2_Data> loadMM2Data(const std::string& path, const bool hiROM);
