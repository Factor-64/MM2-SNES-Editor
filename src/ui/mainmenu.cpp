#include "mainmenu.h"
#include <iostream>

namespace MainMenu {

void Draw(State& state, const bool romLoaded, const bool ready)
{
    if(ImGui::BeginMainMenuBar())
    {
        if(ImGui::BeginMenu("File"))
        {
            if(ImGui::MenuItem("Load ROM File"))
                state.openLoadRom = true;

            // Only show AFTER ROM is loaded
            if(romLoaded)
            {
                ImGui::Separator();

                if(ImGui::MenuItem("Load JSON File"))
                    state.openLoadJson = true;

                if(ImGui::MenuItem("Load Palette File"))
                    state.openLoadPal = true;

                if(ImGui::MenuItem("Export New ROM"))
                    state.openExportRom = true;

                if(ImGui::MenuItem("Show Header Info"))
                    state.openHeaderWindow = true;

                //if(ImGui::MenuItem("Close ROM"))
                //    editor = EditorState{};
            }

            ImGui::EndMenu();
        }

        if (romLoaded && ready)
        {
            if (ImGui::BeginMenu("Export Level Data"))
            {
                if (ImGui::MenuItem("Export Graphics Data"))
                    state.exportGraphics = true;

                if (ImGui::MenuItem("Export MetaTile Data"))
                    state.exportMetaTiles = true;

                if (ImGui::MenuItem("Export MetaTile Palette Data"))
                    state.exportMetaTilePal = true;

                if (ImGui::MenuItem("Export Layout Data"))
                    state.exportLayout = true;

                if (ImGui::MenuItem("Export Scroll Data"))
                    state.exportScroll = true;

                if (ImGui::MenuItem("Export Enemy Data"))
                    state.exportEnemy = true;

                if (ImGui::MenuItem("Export Item Data"))
                    state.exportItem = true;

                if (ImGui::MenuItem("Export Midpoint Data"))
                    state.exportMidpoint = true;

                if (ImGui::MenuItem("Export Pattern Table Settings"))
                    state.exportPatternTable = true;

                if (ImGui::MenuItem("Export Palette Data"))
                    state.exportPalette = true;

                if (ImGui::MenuItem("Export Palette Animation Data"))
                    state.exportPaletteAnimation = true;

                ImGui::Separator();

                if (ImGui::MenuItem("Export All Level Data"))
                    state.exportAllLevelData = true;

                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Import/Export Data"))
            {
                if (ImGui::MenuItem("Import Data"))
                    state.importData = true;

                if (ImGui::MenuItem("Export Common GFX Data"))
                {
                    state.exportGraphics = true;
                    state.exportCommonGFX = true;
                }

                ImGui::EndMenu();
            }
        }

        if (ImGui::BeginMenu("Settings"))
        {
            if (ImGui::MenuItem("Preferences"))
                state.openSettings = true;

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

}
