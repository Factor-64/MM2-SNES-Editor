#pragma once
#include <imgui.h>

enum MenuState {
    MS_NULL,
    MS_OpenLoadROM,
    MS_OpenLoadJson,
    MS_OpenLoadPal,
    MS_ExportGraphics,
    MS_ExportMetaTiles,
    MS_ExportMetaTilePal,
    MS_ExportLayout,
    MS_ExportCollision,
    MS_ExportScroll,
    MS_ExportLayer2Data,
    MS_ExportLayer3Data,
    MS_ExportEnemy,
    MS_ExportItem,
    MS_ExportMidpoint,
    MS_ExportPatternTable,
    MS_ExportPalette,
    MS_ExportLayer2Palette,
    MS_ExportLayer3Palette,
    MS_ExportPaletteAnimation,
    MS_ExportCommonGFX,
    MS_ExportBGGFX,
    MS_ImportData,
    MS_ExportAllData,
    MS_OpenExportROM,
    MS_OpenHeaderWindow
};

void drawMenu(MenuState& state, const bool romLoaded, const bool jsonLoaded, const bool isNES);
