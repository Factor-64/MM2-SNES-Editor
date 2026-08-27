#include "mainmenu.h"
#include <iostream>

void drawMenu(MenuState& state, const bool romLoaded, const bool ready, const bool isNES)
{
    if(ImGui::BeginMainMenuBar())
    {
        bool finished = state == MS_NULL;
        if(ImGui::BeginMenu("File", finished))
        {
            if(ImGui::MenuItem("Load ROM File"))
                state = MS_OpenLoadROM;

            // Only show AFTER ROM is loaded
            if(romLoaded)
            {
                ImGui::Separator();

                if(ImGui::MenuItem("Load JSON File"))
                    state = MS_OpenLoadJson;

                if(ImGui::MenuItem("Load Palette File"))
                    state = MS_OpenLoadPal;

                if (ImGui::MenuItem("Export New ROM"))
                    state = MS_OpenExportROM;

                if (ImGui::MenuItem("Show Header Info"))
                    state = MS_OpenHeaderWindow;

                //if(ImGui::MenuItem("Close ROM"))
                //    editor = EditorState{};
            }

            ImGui::EndMenu();
        }

        if (romLoaded && ready)
        {
            if (ImGui::BeginMenu("Tools"))
            {
                if (ImGui::MenuItem("Graphics Viewer"))
                    state = MS_OpenGraphics;
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Export Level Data", finished))
            {
                if (ImGui::MenuItem("Export Layer 1+2+3 Graphics Data"))
                    state = MS_ExportGraphics;

                if (ImGui::MenuItem("Export MetaTile Data"))
                    state = MS_ExportMetaTiles;

                if (ImGui::MenuItem("Export MetaTile Collision Data", nullptr, false, !isNES))
                    state = MS_ExportCollision;

                if (ImGui::MenuItem("Export MetaTile Palette Data"))
                    state = MS_ExportMetaTilePal;

                if (ImGui::MenuItem("Export Layout Data"))
                    state = MS_ExportLayout;

                if (ImGui::MenuItem("Export Scroll Data"))
                    state = MS_ExportScroll;

                if (ImGui::MenuItem("Export Enemy Data"))
                    state = MS_ExportEnemy;

                if (ImGui::MenuItem("Export Item Data"))
                    state = MS_ExportItem;

                if (ImGui::MenuItem("Export Midpoint Data"))
                    state = MS_ExportMidpoint;

                if (ImGui::MenuItem("Export Pattern Table Settings"))
                    state = MS_ExportPatternTable;

                if (ImGui::MenuItem("Export Palette Data"))
                    state = MS_ExportPalette;

                if (ImGui::MenuItem("Export Palette Animation Data"))
                    state = MS_ExportPaletteAnimation;

                ImGui::Separator();

                if (ImGui::MenuItem("Export All Level Data"))
                    state = MS_ExportAllData;

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Export Background Data", finished && !isNES))
            {
                if (ImGui::MenuItem("Export Layer 2 Tilemap Data"))
                    state = MS_ExportLayer2TilemapData;

                if (ImGui::MenuItem("Export Layer 3 Tilemap Data"))
                    state = MS_ExportLayer3TilemapData;

                if (ImGui::MenuItem("Export Layer 2 Palette Data"))
                    state = MS_ExportLayer2Palette;

                if (ImGui::MenuItem("Export Layer 3 Palette Data"))
                    state = MS_ExportLayer3Palette;

                if (ImGui::MenuItem("Export Scroll Enable Data"))
                    state = MS_ExportBGScrollEnable;

                if (ImGui::MenuItem("Export Scroll Speed Data"))
                    state = MS_ExportBGScrollSpeedData;

                if (ImGui::MenuItem("Export Timemap Mirroring Data"))
                    state = MS_ExportBGTilemapMirroring;

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Import/Export Data", finished))
            {
                if (ImGui::MenuItem("Import Data"))
                    state = MS_ImportData;

                if (ImGui::MenuItem("Export Common GFX Data"))
                    state = MS_ExportCommonGFX;

                ImGui::EndMenu();
            }
        }

        /*if (ImGui::BeginMenu("Settings"))
        {
            if (ImGui::MenuItem("Preferences"))
                state.openSettings = true;

            ImGui::EndMenu();
        }*/

        ImGui::EndMainMenuBar();
    }
}
