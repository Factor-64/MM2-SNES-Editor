#pragma once
#include <imgui.h>

namespace MainMenu {

    struct State {
        bool openLoadRom = false;
        bool openLoadJson = false;
        bool openLoadPal = false;

        bool exportGraphics = false;
        bool exportMetaTiles = false;
        bool exportMetaTilePal = false;
        bool exportLayout = false;
        bool exportScroll = false;
        bool exportEnemy = false;
        bool exportItem = false;
        bool exportMidpoint = false;
        bool exportPatternTable = false;
        bool exportPalette = false;
        bool exportPaletteAnimation = false;
        bool exportCommonGFX = false;
        bool importData = false;
        bool exportAllLevelData = false;

        bool openExportRom = false;
        bool openSettings = false;
        bool openHeaderWindow = false;
    };

    void Draw(State& state, const bool romLoaded, const bool jsonLoaded);

}
