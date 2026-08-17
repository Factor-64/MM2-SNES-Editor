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
#include "game/level.h"
#include "game/checkpoint.h"

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
        VM_Layer2,
        VM_Layer3,
    };

    enum EditMode : int {
        EM_Metatiles = 0,
        EM_Level,
        EM_Collision,
        EM_Layer2,
        EM_Layer3
    };

    enum LvlViewMode : int {
        LVM_Level = 0,
        LVM_Objects,
        LVM_Checkpoints
    };

    enum ActiveWindow : int {
        AW_None,
        AW_Palette,
        AW_Tileset,
        AW_Editor
    };

    enum PaletteType : int {
        PT_Normal,
        PT_Animated,
        PT_Layer2,
        PT_Layer3
    };

    struct PaletteClipboard {
        bool hasData = false;
        Palette colors;
    };

    struct ColorClipboard {
        bool hasData = false;
        ColorRGBA color;
    };

    struct ObjClipboard {
        bool hasData = false;
        Object obj;
        int type = 0;
    };

    struct EditorState {
        bool jsonLoaded = false;
        bool romLoaded = false;
        bool paletteLoaded = false;
        bool isHiROM = false;
        bool dragging = false;
        bool rebuildTileset = true;
        bool rebuildView = true;
        bool rebuildEdit = true;
        bool rebuildData = true;
        bool universalBGColor = false;
        bool animatePalettes = false;
        bool inLevelRegion = false;
        bool paintMode = false;
        bool hFlip = false;
        bool vFlip = false;
        bool hPriority = false;
        bool previewScroll = false;
        bool scrollLayer2Vertical = true;
        bool scrollLayer3Vertical = true;
        bool rebuildBackgrounds = false;
        int currentScreen = 0;
        int currentScreenId = 0;
        int screenCount = 0;

        int tilesetZoom = 1;
        int editorZoom = 1;

        int layer2Scanlines = 0;
        int layer3Scanlines = 0;
        ActiveWindow activeWindow = AW_None;

        std::vector<uint8_t> rom;
        std::array<ColorRGBA, 64> nesMasterPalette;
        Palettes palettes;
        Palettes aniPalettes;
        Palettes subPalettes;

        PaletteType paletteType;

        int mode = 1;

        SNESHeader header{};
        
        //TODO convert vectors to arrays as most of these are fixed sizes
        std::vector<MM2_Data> data;
        std::vector<Tile> levelTiles;
        std::vector<Tile> layer3Tiles;
        TileMap levelTileMap;
        TileMap layer2TileMap;
        TileMap layer3TileMap;
        std::vector<MacroTile> levelMacroTiles;
        std::vector<MetaTile> levelMetaTiles;
        std::vector<uint8_t> levelData;
        std::vector<BGTileData> layer2TileData;
        std::vector<BGTileData> layer3TileData;
        std::vector<uint8_t> scrollData;
        std::array<ScrollEnable, 64> bgScrollData;
        std::array<BGPositionData, 3> bgPositionData;
        std::array<BGTilemapMirror, 64> bgTilemapMirror;
        std::array<std::array<BGSpeedData, 4>, 32> bgScrollSpeeds;
        Objects itemData;
        Objects enemyData;
        Checkpoints checkpointData;

        TilemapTexture tileset;

        int selectedLevel = 0;
        ViewMode tileViewMode = VM_Tileset;
        EditMode editMode = EM_Metatiles;
        LvlViewMode lvlViewMode = LVM_Level;
        int selectedColor = 0;
        int paletteIndex = 0;
        int subPaletteIndex = 0;
        int aniPalIndex = 0;
        int selectedTile = -1;
        int editingPalette = -1;
        int editingColor = -1;
        int selectedObject = -1;
        int objectType = -1;

        PaletteClipboard palClip;
        ColorClipboard colClip;
        ObjClipboard objClip;

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
    void drawScrollData();
    void drawBGScrollData();
    void saveROMData();
    void drawLevelView();
    void drawTileView();
    void SelectTileFromClick(int tileX, int tileY, int atlasWidth);
    void PaintTileGeneric(int tileX, int tileY, int atlasWidth, const bool color);
    void PaintTilePixel(int tileIndex, int x, int y);
    void PaintMacroTilePixel(int macroIndex, int x, int y);
    void DrawColorButton(const std::string& id, ColorRGBA& col, const PaletteType type, size_t paletteIndex, int colorIndex, const char* popupName, const LevelEntry& level, ImVec2 size = ImVec2(0, 0));
    void DrawPaletteRow(const char* label, size_t index, Palette& pal, int colorsPerPalette, const PaletteType type, const char* popupName, const char* ident, const LevelEntry& level);
    void DrawAllPalettes(int colorsPerPalette, const char* popupName, const LevelEntry& level);
    void DrawAnimatedPalettes(int colorsPerPalette, int colorsPerFrame, const char* popupName, const LevelEntry& level);
    void DrawNESPopup(const LevelEntry& level);
    void DrawSNESPopup(const LevelEntry& level);
    int findNESIndexFromRGBA(const ColorRGBA& col);
    void writeNESColorToROM(const LevelEntry& level, int index);
    void writeSNESColorToROM(const LevelEntry& level);
    void writeNESPaletteToROM(size_t paletteIndex, const Palette& pal, const LevelEntry& level);
    void writeSNESPaletteToROM(size_t paletteIndex, const Palette& pal, const LevelEntry& level);
    void updatePaletteAnimation();
    void applyAnimationFrame();
    void saveBinary(const LevelField field, const int lvl, const int mode);
    void drawEditMode();
    void PaintTileBackground(std::vector<BGTileData>& data, int tileX, int tileY, int atlasWidth, const bool color, const bool subPal);
    void updateScollPreview();

    std::vector<uint8_t> exportData;
    int currentExportIndex = -1;

    MenuState menuState = MS_NULL;
    EditorState editor;

    double animAccumulator = 0.0;
    uint64_t lastTime = SDL_GetPerformanceCounter();

    bool open = true;
    bool openHeader = false;
    bool exportingAllData = false;
    bool exportingData = false;
};
