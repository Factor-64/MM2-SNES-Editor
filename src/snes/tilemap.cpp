#include "tilemap.h"
#include <print>

TileMap makeTileMap(const std::vector<Tile>& atlas, int mapWidth, uint8_t shape)
{
    TileMap map;
    map.width = mapWidth;
    map.shape = shape;

    int tilesPerMeta =
        (shape == 2 ? 4 :
            shape == 1 ? 2 : 1); // 0:1, 1:2, 2:4

    int metaCount = static_cast<int>((atlas.size() + tilesPerMeta - 1) / tilesPerMeta);
    map.height = (metaCount + mapWidth - 1) / mapWidth;

    map.cells.resize(map.width * map.height);

    int idx = 0;
    for (int i = 0; i < metaCount; ++i)
    {
        int x = i % mapWidth;
        int y = i / mapWidth;
        if (y >= map.height) break;

        TileRef& ref = map.at(x, y);
        ref.index = static_cast<uint16_t>(i * tilesPerMeta);
    }

    return map;
}

void blitTileToBuffer(const Tile& tile, uint8_t* buffer, int bufW, int dstX, int dstY)
{
    for(int y = 0; y < 8; ++y)
    {
        for(int x = 0; x < 8; ++x)
        {
            buffer[(dstY + y) * bufW + (dstX + x)] = tile.pixels[y * 8 + x];
        }
    }
}

void blitTileToRGBA(
    const Tile& tile,
    const Palette& palette,
    const ColorRGBA bgColor,
    ColorRGBA* buffer,
    int bufW,
    int dstX,
    int dstY)
{
    for (int y = 0; y < 8; ++y)
    {
        for (int x = 0; x < 8; ++x)
        {
            uint8_t index = tile.pixels[y * 8 + x];
            if (index >= 16) index = 0;

            int dst = (dstY + y) * bufW + (dstX + x);
            if (index == 0 && bgColor.a != 0)
                buffer[dst] = bgColor;
            else
                buffer[dst] = palette[index];
        }
    }
}

void renderTileMapToRGBA(
    const TileMap& map,
    const std::vector<Tile>& atlas,
    const Palette& palette,
    const ColorRGBA& bgColor,
    std::vector<ColorRGBA>& outPixels,
    int& width,
    int& height)
{
    int tileW = (map.shape == 2 ? 16 : 8);
    int tileH = (map.shape == 1 ? 16 : map.shape == 2 ? 16 : 8);

    int bufW = map.width * tileW;
    int bufH = map.height * tileH;

    width = bufW;
    height = bufH;

    outPixels.resize(bufW * bufH);

    for (int my = 0; my < map.height; ++my)
    {
        for (int mx = 0; mx < map.width; ++mx)
        {
            const TileRef& ref = map.at(mx, my);
            int base = ref.index;

            int px = mx * tileW;
            int py = my * tileH;

            switch (map.shape)
            {
            case 0: // 8x8
                if (base >= static_cast<int>(atlas.size())) continue;
                blitTileToRGBA(atlas[base], palette, bgColor, outPixels.data(), bufW, px, py);
                break;

            case 1: // 8x16 (2 tiles: top, bottom)
                if (base + 1 >= static_cast<int>(atlas.size())) continue;
                blitTileToRGBA(atlas[base], palette, bgColor, outPixels.data(), bufW, px, py);
                blitTileToRGBA(atlas[base + 1], palette, bgColor, outPixels.data(), bufW, px, py + 8);
                break;

            case 2: // 16x16 (4 tiles: TL, TR, BL, BR)
                if (base + 3 >= static_cast<int>(atlas.size())) continue;
                blitTileToRGBA(atlas[base], palette, bgColor, outPixels.data(), bufW, px, py);
                blitTileToRGBA(atlas[base + 1], palette, bgColor, outPixels.data(), bufW, px + 8, py);
                blitTileToRGBA(atlas[base + 2], palette, bgColor, outPixels.data(), bufW, px, py + 8);
                blitTileToRGBA(atlas[base + 3], palette, bgColor, outPixels.data(), bufW, px + 8, py + 8);
                break;
            }
        }
    }
}

void uploadTilemapTextureRGBA(const std::vector<ColorRGBA>& pixels, TilemapTexture& t)
{
    glGenTextures(1, &t.tex);
    glBindTexture(GL_TEXTURE_2D, t.tex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        t.width,
        t.height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        pixels.data()
        );

    glBindTexture(GL_TEXTURE_2D, 0);
}

std::vector<MacroTile> buildMacroTiles(const TileMap& map) 
{
    std::vector<MacroTile> out;

    if (map.shape != 1) return out; // only valid for 8x16 mode

    int macroWidth = map.width / 2;
    int macroHeight = map.height;

    out.reserve(macroWidth * macroHeight);

    for (int y = 0; y < macroHeight; ++y) 
    {
        for (int x = 0; x < macroWidth; ++x) 
        {
            MacroTile t;
            t.left = map.at(x * 2, y);
            t.right = map.at(x * 2 + 1, y);
            out.push_back(t);
        }
    }

    return out;
}

void blitMacroTileToRGBA(
    const MacroTile& macro,
    const std::vector<Tile>& atlas,
    const Palette& palette,
    const ColorRGBA& bgColor,
    ColorRGBA* out,
    int bufW,
    int px,
    int py)
{
    // LEFT 8×16
    if (macro.left.index + 1 >= static_cast<int>(atlas.size())) return;
    blitTileToRGBA(atlas[macro.left.index + 0], palette, bgColor, out, bufW, px, py);
    blitTileToRGBA(atlas[macro.left.index + 1], palette, bgColor, out, bufW, px, py + 8);

    // RIGHT 8×16
    if (macro.right.index + 1 >= static_cast<int>(atlas.size())) return;
    blitTileToRGBA(atlas[macro.right.index + 0], palette, bgColor, out, bufW, px + 8, py);
    blitTileToRGBA(atlas[macro.right.index + 1], palette, bgColor, out, bufW, px + 8, py + 8);
}

void blitMetaTileToRGBA(
    const MetaTile& meta,
    const std::vector<Tile>& atlas,
    const Palettes& palettes,
    const uint8_t palOffset,
    const ColorRGBA& bgColor,
    ColorRGBA* out,
    int bufW,
    int px,
    int py)
{
    // Top-left
    blitMacroTileToRGBA(meta.tiles[0], atlas, palettes[meta.palettes[0] + palOffset], bgColor, out, bufW, px, py);

    // Top-right
    blitMacroTileToRGBA(meta.tiles[1], atlas, palettes[meta.palettes[1] + palOffset], bgColor, out, bufW, px + 16, py);

    // Bottom-left
    blitMacroTileToRGBA(meta.tiles[2], atlas, palettes[meta.palettes[2] + palOffset], bgColor, out, bufW, px, py + 16);

    // Bottom-right
    blitMacroTileToRGBA(meta.tiles[3], atlas, palettes[meta.palettes[3] + palOffset], bgColor, out, bufW, px + 16, py + 16);
}

void renderMetaTileMapToRGBA(
    const std::vector<MetaTile>& metaTiles,
    int metaWidth,
    const std::vector<Tile>& atlas,
    const Palettes& palettes,
    const uint8_t palOffset,
    const ColorRGBA& bgColor,
    std::vector<ColorRGBA>& outPixels,
    int& width,
    int& height)
{

    if (metaWidth <= 0 || metaTiles.empty()) return;

    if (metaTiles.size() % metaWidth != 0)
        return;

    int tileW = 32;
    int tileH = 32;

    int metaHeight = metaTiles.size() / metaWidth;

    width = metaWidth * tileW;
    height = metaHeight * tileH;

    outPixels.resize(width * height);

    for (int my = 0; my < metaHeight; ++my)
    {
        for (int mx = 0; mx < metaWidth; ++mx)
        {
            const MetaTile& meta = metaTiles[my * metaWidth + mx];

            int px = mx * tileW;
            int py = my * tileH;

            blitMetaTileToRGBA(
                meta,
                atlas,
                palettes,
                palOffset,
                bgColor,
                outPixels.data(),
                width,
                px,
                py
            );
        }
    }
}

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
    int& outH)
{
    const int tileW = 32;
    const int tileH = 32;

    int fullMetaHeight = metaIndices.size() / fullMetaWidth;

    outW = windowWidth * tileW;
    outH = fullMetaHeight * tileH;

    outPixels.resize(outW * outH);

    for (int my = 0; my < fullMetaHeight; ++my)
    {
        for (int mx = 0; mx < windowWidth; ++mx)
        {
            int srcX = windowX + mx;
            int srcIndex = my * fullMetaWidth + srcX;

            uint8_t idx = metaIndices[srcIndex];
            const MetaTile& meta = metaTiles[idx];

            int px = mx * tileW;
            int py = my * tileH;

            blitMetaTileToRGBA(
                meta,
                atlas,
                palettes,
                palOffset,
                bgColor,
                outPixels.data(),
                outW,
                px,
                py
            );
        }
    }
}

Tile applyFlips(const Tile& src, bool hFlip, bool vFlip)
{
    Tile t = src;

    for (int y = 0; y < 8; ++y)
    {
        for (int x = 0; x < 8; ++x)
        {
            int sx = hFlip ? (7 - x) : x;
            int sy = vFlip ? (7 - y) : y;
            t.pixels[y * 8 + x] = src.pixels[sy * 8 + sx];
        }
    }
    return t;
}

void renderBGTileMapToRGBA(
    const std::vector<BGTileData>& bgTileData,
    int screenWidth, // in tiles
    const std::vector<Tile>& atlas,
    const Palettes& palettes,
    const ColorRGBA& bgColor,
    std::vector<ColorRGBA>& outPixels,
    int& outW,
    int& outH)
{
    int rows = (bgTileData.size() + screenWidth - 1) / screenWidth;
    outW = screenWidth * 8;
    outH = rows * 8;

    outPixels.resize(outW * outH, bgColor);

    for (int i = 0; i < bgTileData.size(); ++i)
    {
        const BGTileData& td = bgTileData[i];

        int tileX = (i % screenWidth) * 8;
        int tileY = (i / screenWidth) * 8;

        int id = td.tileId + (td.vramPage * 256);

        const Tile& srcTile = atlas[id];

        Tile tile = applyFlips(srcTile, td.hFlip, td.vFlip);

        const Palette& pal = palettes[td.palette];

        blitTileToRGBA(
            tile,
            pal,
            bgColor,
            outPixels.data(),
            outW,
            tileX,
            tileY
        );
    }
}
