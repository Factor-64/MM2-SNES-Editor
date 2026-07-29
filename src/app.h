#pragma once
#include <SDL3/SDL.h>
#include <vector>
#include "mm2data.h"
#include "snes/palette.h"
#include "snes/snesheaderreader.h"
#include "ui/mainmenu.h"
#include "snes/tiledecoder.h"
#include "snes/tilemap.h"
#include "game/objects.h"

class App {
public:
    App();
    ~App();
    void run();

private:
    struct FlagItem {
        const char* name;
        uint8_t bit;
    };

    enum ViewMode : int {
        VM_Tileset = 0,
        VM_Metatiles,
        VM_Collision,
        VM_Invalid
    };

    enum ActiveWindow : int {
        AW_None,
        AW_Palette,
        AW_Tileset,
        AW_Editor
    };

    struct PaletteClipboard {
        bool hasData = false;
        Palette colors;
    };

    struct ColorClipboard {
        bool hasData = false;
        ColorRGBA color;
    };

    struct EditorState {
        bool jsonLoaded = false;
        bool romLoaded = false;
        bool paletteLoaded = false;
        bool isHiROM = false;
        bool editingAniPal = false;
        bool dragging = false;
        bool rebuildTileset = true;
        bool rebuildView = true;
        bool rebuildData = true;
        bool univeralBGColor = false;
        bool animatePalettes = false;
        bool inLevelRegion = false;

        int tilesetZoom = 1;
        int editorZoom = 1;
        ActiveWindow activeWindow = AW_None;

        std::vector<uint8_t> rom;
        std::array<ColorRGBA, 64> nesMasterPalette;
        Palettes palettes;
        Palettes aniPalettes;

        int mode = 1;

        SNESHeader header{};
        
        std::vector<MM2_Data> data;
        std::vector<Tile> levelTiles;
        TileMap levelTileMap;
        std::vector<MacroTile> levelMacroTiles;
        std::vector<MetaTile> levelMetaTiles;
        std::vector<uint8_t> levelData;
        std::vector<uint8_t> scrollData;
        Objects itemData;
        Objects enemyData;

        TilemapTexture tileset;

        int selectedLevel = 0;
        ViewMode tileViewMode = VM_Metatiles;
        int selectedColor = 0;
        int paletteIndex = 0;
        int aniPalIndex = 0;
        int selectedTile = -1;
        int editingPalette = -1;
        int editingColor = -1;
        int lvlViewMode = 0;
        int selectedObject = -1;
        int objectType = -1;
        int animatedFrame = 0;

        PaletteClipboard palClip;
        ColorClipboard colClip;

        ImVec2 dragOffset;
        std::string romName;
        ColorRGBA tempColor;

        PalAnimation animate;
        int animFrame = 0;
        int animTimer = 0;
    };

    SDL_Window* window = nullptr;
    SDL_GLContext glContext = nullptr;

    void initSDL();
    void initGL();
    void initImGui();
    void shutdown();
    void drawTilesetWindow();
    void drawPaletteWindow();
    void drawLevelWindow();
    void drawHeaderWindow();
    bool drawScrollData(std::vector<uint8_t>& data, int screenNum);
    void saveROMData();
    void drawLevelView();
    void drawTileView(TilemapTexture& tileset, const ViewMode view, const int zoom);
    void SelectTileFromClick(int tileX, int tileY, int atlasWidth);
    void PaintTileGeneric(int tileX, int tileY, int atlasWidth, const bool color);
    void PaintTilePixel(int tileIndex, int x, int y);
    void DrawColorButton(const std::string& id, ColorRGBA& col, bool isAni, size_t paletteIndex, int colorIndex, const char* popupName, const LevelEntry& level, ImVec2 size = ImVec2(0, 0));
    void DrawPaletteRow(const char* label, size_t index, Palette& pal, int colorsPerPalette, bool isAni, const char* popupName, const char* ident, const LevelEntry& level);
    void DrawAllPalettes(int colorsPerPalette, const char* popupName, const LevelEntry& level);
    void DrawAnimatedPalettes(int colorsPerPalette, int colorsPerFrame, const char* popupName, const LevelEntry& level);
    void DrawNESPopup(const LevelEntry& level);
    void DrawSNESPopup(const LevelEntry& level);
    int findNESIndexFromRGBA(const ColorRGBA& col);
    void writeNESColorToROM(const LevelEntry& level, int index);
    void writeSNESColorToROM(const LevelEntry& level);
    void writeNESPaletteToROM(size_t paletteIndex, const Palette& pal, const bool isAni, const LevelEntry& level);
    void writeSNESPaletteToROM(size_t paletteIndex, const Palette& pal, const bool isAni, const LevelEntry& level);
    void updatePaletteAnimation();
    void applyAnimationFrame();
    void saveBinary(const LevelField field, const int lvl, const int mode);

    std::vector<uint8_t> exportData;
    int currentExportIndex = -1;

    MainMenu::State menuState;
    EditorState editor;

    double animAccumulator = 0.0;
    uint64_t lastTime = SDL_GetPerformanceCounter();

    bool open = true;
    bool exportGFX = false;
    bool exportingData = false;
};
