#include "mm2data.h"
#include "snes/address.h"
#include <fstream>
#include <stdexcept>

static uint32_t parseAddr(const std::string& s, const bool hiROM)
{
    auto colon = s.find(':');
    if (colon == std::string::npos)
        throw std::runtime_error("Invalid SNES address: " + s);

    uint32_t bank = std::stoul(s.substr(0, colon), nullptr, 16);
    uint32_t addr = std::stoul(s.substr(colon + 1), nullptr, 16);

    return snesBankAddrToPc(bank, addr, hiROM);
}

static Range parseRange(const std::string& s, const bool hiROM)
{
    auto dash = s.find('-');
    if (dash == std::string::npos)
        throw std::runtime_error("Invalid range: " + s);

    Range r{};
    r.start = parseAddr(s.substr(0, dash), hiROM);
    r.end   = parseAddr(s.substr(dash + 1), hiROM);

    return r;
}

static GFXData parseGfxEntry(const json& j, const bool hiROM)
{
    GFXData out;
    
    if (j.contains("layer12"))
    {
        auto r = j["layer12"];

        if (r.is_string())
        {
            out.layer12.push_back(parseRange(r.get<std::string>(), hiROM));
        }
        else if (r.is_array())
        {
            for (auto& e : r)
                out.layer12.push_back(parseRange(e.get<std::string>(), hiROM));
        }
        else if (r.is_object())
        {
            for (auto it = r.begin(); it != r.end(); ++it)
                out.layer12.push_back(parseRange(it.value().get<std::string>(), hiROM));
        }

        if (j.contains("layer3"))
        {
            r = j["layer3"];
            if (r.is_string())
            {
                out.layer3.push_back(parseRange(r.get<std::string>(), hiROM));
            }
            else if (r.is_array())
            {
                for (auto& e : r)
                    out.layer3.push_back(parseRange(e.get<std::string>(), hiROM));
            }
            else if (r.is_object())
            {
                for (auto it = r.begin(); it != r.end(); ++it)
                    out.layer3.push_back(parseRange(it.value().get<std::string>(), hiROM));
            }
        }
    }
    else
    {
        if (j.is_string())
        {
            out.layer12.push_back(parseRange(j.get<std::string>(), hiROM));
        }
        else if (j.is_array())
        {
            for (auto& e : j)
                out.layer12.push_back(parseRange(e.get<std::string>(), hiROM));
        }
        else if (j.is_object())
        {
            for (auto it = j.begin(); it != j.end(); ++it)
                out.layer12.push_back(parseRange(it.value().get<std::string>(), hiROM));
        }
    }
    if (j.contains("common"))
    {
        const auto& c = j["common"];

        if (c.is_number_integer())
        {
            out.commonIdx.push_back(c.get<int>());
        }
        else if (c.is_array())
        {
            for (const auto& v : c)
            {
                if (!v.is_number_integer())
                    throw std::runtime_error("common array contains non-integer");

                out.commonIdx.push_back(v.get<int>());
            }
        }
        else
        {
            throw std::runtime_error("Invalid type for 'common'");
        }
    }

    return out;
}

static uint32_t getAddr(const json& j, const char* key, const bool hiROM)
{
    return parseAddr(j.at(key).get<std::string>(), hiROM);
}

static LevelEntry parseLevel(const json& j, const bool hiROM)
{
    LevelEntry L{};

    // level data
    L.chip32x32         = getAddr(j, "chip32x32", hiROM);
    L.chip32x32_palette = getAddr(j, "chip32x32_palette", hiROM);
    L.map               = getAddr(j, "map", hiROM);
    L.scroll            = getAddr(j, "scroll", hiROM);

    L.collision = 0;
    if (j.contains("collision"))
        L.collision = getAddr(j, "collision", hiROM);

    // bg data
    L.bg_tilemap = 0;
    //L.bg_boss = 0;
    //L.bg_checkpoint = 0;
    L.bg_mirror = 0;
    L.bg_scroll = 0;
    L.bg_speed = 0;
    L.bg_start = 0;
    if (j.contains("background"))
    {
        auto& B = j["background"];
        L.bg_tilemap    = parseAddr(B["tilemap"], hiROM);
        L.bg_scroll     = parseAddr(B["scroll_flags"], hiROM);
        L.bg_start = parseAddr(B["start"], hiROM);
        //L.bg_checkpoint = parseAddr(B["checkpoint"], hiROM);
        //L.bg_boss       = parseAddr(B["boss"], hiROM);
        L.bg_mirror     = parseAddr(B["mirror"], hiROM);
        L.bg_speed      = parseAddr(B["speed"], hiROM);
    }

    // enemy
    L.enemy_screen = getAddr(j["enemy"], "screen", hiROM);
    L.enemy_x    = getAddr(j["enemy"], "x", hiROM);
    L.enemy_y    = getAddr(j["enemy"], "y", hiROM);
    L.enemy_type = getAddr(j["enemy"], "type", hiROM);

    // item
    L.item_screen = getAddr(j["item"], "screen", hiROM);
    L.item_x    = getAddr(j["item"], "x", hiROM);
    L.item_y    = getAddr(j["item"], "y", hiROM);
    L.item_type = getAddr(j["item"], "type", hiROM);

    // midpoint
    L.midpoint_start_y          = getAddr(j["midpoint"], "start_y", hiROM);
    /*L.midpoint_start_room = getAddr(j["midpoint"], "start_screen", hiROM);
    L.midpoint_enemy_index      = getAddr(j["midpoint"], "enemy_index", hiROM);
    L.midpoint_item_index       = getAddr(j["midpoint"], "item_index", hiROM);
    L.midpoint_map_back_high    = getAddr(j["midpoint"], "map_back_high", hiROM);
    L.midpoint_map_back_low     = getAddr(j["midpoint"], "map_back_low", hiROM);
    L.midpoint_map_forward_high = getAddr(j["midpoint"], "map_forward_high", hiROM);
    L.midpoint_map_forward_low  = getAddr(j["midpoint"], "map_forward_low", hiROM);
    L.midpoint_start_scroll     = getAddr(j["midpoint"], "start_scroll", hiROM);
    L.midpoint_left_room        = getAddr(j["midpoint"], "left_screen", hiROM);
    L.midpoint_right_room       = getAddr(j["midpoint"], "right_screen", hiROM);*/

    // pattern table
    L.pattern = parseAddr(j["pattern_table_setting"].get<std::string>(), hiROM);

    // palette
    auto& P = j["palette"];

    L.palette_afc = parseAddr(P["anime_frame_count"], hiROM);

    L.palette_aft = parseAddr(P["anime_frame_timer"], hiROM);

    L.palette_data = parseAddr(P["data"], hiROM);

    L.palette_anime = parseAddr(P["anime_palette"], hiROM);

    std::string value = P.value("layer2", "");

    L.palette_layer2 = 0;
    L.palette_layer3 = 0;
    if(!value.empty())
        L.palette_layer2 = parseAddr(value, hiROM);

    value = P.value("layer3", "");
    if (!value.empty())
        L.palette_layer3 = parseAddr(value , hiROM);

    return L;
}

std::vector<MM2_Data> loadMM2Data(const std::string& path, const bool hiROM)
{
    std::ifstream f(path);
    if(!f)
        throw std::runtime_error("Cannot open JSON: " + path);

    json root;
    f >> root;

    const json& modes = root.at("modes");

    std::vector<MM2_Data> final;
    final.resize(modes.size());

    for (size_t i = 0; i < modes.size(); ++i)
    {
        const json& jmode = root.at(modes[i]);

        MM2_Data out{};
        
        out.modeName = modes[i];

        // GFX
        for (auto it = jmode["gfx"].begin(); it != jmode["gfx"].end(); ++it)
            out.gfx[it.key()] = parseGfxEntry(it.value(), hiROM);

        // Levels
        for (auto it = jmode["levels"].begin(); it != jmode["levels"].end(); ++it)
        {
            const std::string name = it.key();
            out.levels[name] = parseLevel(it.value(), hiROM);
            out.levelNames.push_back(name);
        }

        final[i] = out;
    }

    return final;
}
