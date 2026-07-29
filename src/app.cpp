#include "app.h"
#include <imgui.h>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_opengl3.h>
#include <glad/glad.h>
#include <ImGuiFileDialog.h>
#include "game/level.h"
#include "game/collision.h"
#include <bit>
#include "snes/address.h"
#include <print>

App::App()
{
    initSDL();
    initGL();
    initImGui();
}

App::~App()
{
    shutdown();
}

void App::initSDL()
{
    if(!SDL_Init(SDL_INIT_VIDEO))
        throw std::runtime_error("Failed to initialize SDL");

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    window = SDL_CreateWindow("SNTE", 1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);

    if(!window)
        throw std::runtime_error("Failed to create SDL window");

    glContext = SDL_GL_CreateContext(window);
    if(!glContext)
        throw std::runtime_error("Failed to create OpenGL context");

    SDL_GL_MakeCurrent(window, glContext);
}

void App::initGL()
{
    if(!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
        throw std::runtime_error("Failed to initialize GLAD");

    glViewport(0, 0, 1280, 720);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_DEPTH_TEST);
}

void App::initImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    
    ImGui_ImplSDL3_InitForOpenGL(window, glContext);
    ImGui_ImplOpenGL3_Init("#version 460");
    
    ImGui::GetIO().ConfigWindowsMoveFromTitleBarOnly = true;
}

void App::run()
{
    bool quit = false;
    SDL_Event e;

    while(!quit)
    {
        uint64_t now = SDL_GetPerformanceCounter();
        double delta = (double)(now - lastTime) / SDL_GetPerformanceFrequency();
        lastTime = now;

        animAccumulator += delta;

        while (animAccumulator >= (1.0 / 60.0))
        {
            updatePaletteAnimation();
            animAccumulator -= (1.0 / 60.0);
        }

        while(SDL_PollEvent(&e))
        {
            ImGui_ImplSDL3_ProcessEvent(&e);

            if(e.type == SDL_EVENT_QUIT)
                quit = true;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();

        ImGui::NewFrame();

        ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::DockSpaceOverViewport(vp->ID);

        MainMenu::Draw(menuState, editor.romLoaded, editor.jsonLoaded && editor.paletteLoaded);

        if(menuState.openLoadJson)
        {
            if(!ImGuiFileDialog::Instance()->IsOpened())
            {
                IGFD::FileDialogConfig config;
                config.countSelectionMax = 1;

                ImGuiFileDialog::Instance()->OpenDialog(
                    "ChooseJSON",
                    "Select JSON File",
                    "JSON (*.json){.json},All files (*.*){.*}",
                    config
                );
            }
            menuState.openLoadJson = false;
        }

        if(menuState.openLoadRom)
        {
            if(!ImGuiFileDialog::Instance()->IsOpened())
            {
                IGFD::FileDialogConfig config;
                config.countSelectionMax = 1;

                ImGuiFileDialog::Instance()->OpenDialog(
                    "ChooseROM",
                    "Select ROM File",
                    "SNES ROMs (*.sfc *.smc){.sfc,.smc},Binary (*.bin){.bin},All files (*.*){.*}",
                    config
                    );
            }
            menuState.openLoadRom = false;
        }

        if(menuState.openLoadPal)
        {
            if(!ImGuiFileDialog::Instance()->IsOpened())
            {
                IGFD::FileDialogConfig config;
                config.countSelectionMax = 1;

                ImGuiFileDialog::Instance()->OpenDialog(
                    "ChoosePalette",
                    "Select Palette File",
                    "Palette (*.pal){.pal},Binary (*.bin){.bin},All files (*.*){.*}",
                    config
                    );
            }
            menuState.openLoadPal = false;
        }

        if (menuState.openExportRom)
        {
            if (!ImGuiFileDialog::Instance()->IsOpened())
            {
                IGFD::FileDialogConfig config;
                config.countSelectionMax = 1;
                config.fileName = editor.romName;
                config.flags = ImGuiFileDialogFlags_ConfirmOverwrite;

                ImGuiFileDialog::Instance()->OpenDialog(
                    "SaveROM",
                    "Save ROM As...",
                    "SNES ROM (*.sfc){.sfc},All files (*.*){.*}",
                    config
                );
            }
            menuState.openExportRom = false;
        }

        if(ImGuiFileDialog::Instance()->IsOpened())
        {
            ImVec2 vp = ImGui::GetMainViewport()->Size;
            ImVec2 minSize = ImVec2(vp.x * 0.6f, vp.y * 0.6f);
            std::string key = ImGuiFileDialog::Instance()->GetOpenedKey();

            if(ImGuiFileDialog::Instance()->Display(
                    key,
                    ImGuiWindowFlags_NoCollapse,
                    minSize,
                    vp
                    ))
            {
                if(ImGuiFileDialog::Instance()->IsOk())
                {
                    std::string path = ImGuiFileDialog::Instance()->GetFilePathName();

                    if(key == "ChooseROM")
                    {
                        EditorState e{};
                        editor = e;
                        editor.romName = ImGuiFileDialog::Instance()->GetCurrentFileName();
                        std::ifstream f(path, std::ios::binary);
                        editor.rom.clear();
                        editor.rom.assign(std::istreambuf_iterator<char>(f), {});
                        if (editor.rom.size() % 0x4000 == 512)
                            editor.rom.erase(editor.rom.begin(), editor.rom.begin() + 512);
                        editor.header = readSNESHeader(editor.rom);
                        editor.isHiROM = (editor.header.mapMode & 0x10) != 0;
                        editor.romLoaded = true;
                        editor.rebuildData = true;
                    }
                    else if(key == "ChooseJSON")
                    {
                        editor.data = loadMM2Data(path, editor.isHiROM);
                        editor.jsonLoaded = true;
                        editor.rebuildData = true;
                    }
                    else if(key == "ChoosePalette")
                    {
                        std::ifstream f(path, std::ios::binary);
                        editor.nesMasterPalette = decodeNESMasterPalette(std::vector<uint8_t>(std::istreambuf_iterator<char>(f), {}));
                        editor.paletteLoaded = true;
                        editor.rebuildData = true;
                    }
                    else if (key == "SaveROM")
                    {
                        saveROMData();

                        std::ofstream out(path, std::ios::binary);
                        out.write(reinterpret_cast<const char*>(editor.rom.data()), editor.rom.size());
                    }
                    else if (key == "SaveBinary")
                    {
                        std::ofstream out(path, std::ios::binary);
                        out.write(reinterpret_cast<const char*>(exportData.data()), exportData.size());
                        exportData.clear();
                        if (currentExportIndex > -1)
                        {
                            if (exportGFX)
                                menuState.exportGraphics = true;
                            else
                                menuState.exportPaletteAnimation = true;
                            ++currentExportIndex;
                        }
                        else if (currentExportIndex < -1)
                        {
                            menuState.exportPaletteAnimation = true;
                        }
                        else
                        {
                            exportingData = false;
                        }
                    }
                    else if (key == "ChooseData")
                    {
                        auto paths = ImGuiFileDialog::Instance()->GetSelection();
                        for (auto& sel : paths)
                        {
                            const std::string& fileName = sel.first;
                            const std::string& spath = sel.second;
                            size_t pos = fileName.rfind('_');
                            //std::string levelName = fileName.substr(0, pos);
                            std::string address = fileName.substr(pos + 1);
                            uint32_t addr = std::stoul(address, nullptr, 16);
                            addr = snesToPc(addr, editor.isHiROM);
                            std::ifstream f(spath, std::ios::binary);
                            std::vector<uint8_t> data((std::istreambuf_iterator<char>(f)),
                                std::istreambuf_iterator<char>());
                            if (addr + data.size() > editor.rom.size())
                            {
                                editor.rom.resize(addr + data.size());
                            }
                            std::copy(data.begin(), data.end(), editor.rom.begin() + addr);
                            editor.rebuildData = true;
                        }
                    }
                }
                else
                {
                    menuState.exportGraphics = false;
                    menuState.exportPaletteAnimation = false;
                    exportGFX = false;
                    exportingData = false;
                    menuState.exportCommonGFX = false;
                    currentExportIndex = -1;
                    menuState.exportAllLevelData = false;
                }

                ImGuiFileDialog::Instance()->Close();
            }
        }

        if(editor.jsonLoaded && editor.romLoaded && editor.paletteLoaded)
        {
            bool ctrl = ImGui::GetIO().KeyCtrl;
            int zoomDelta = 0;

            if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Equal)) zoomDelta = +1;
            if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Minus)) zoomDelta = -1;

            if (zoomDelta != 0)
            {
                switch (editor.activeWindow)
                {
                case AW_Tileset:
                    editor.tilesetZoom += zoomDelta;
                    if (editor.tilesetZoom < 1)
                        editor.tilesetZoom = 1;
                    else if (editor.tilesetZoom > 8)
                        editor.tilesetZoom = 8;
                    break;

                case AW_Editor:
                    editor.editorZoom += zoomDelta;
                    if (editor.editorZoom < 1)
                        editor.editorZoom = 1;
                    else if (editor.editorZoom > 8)
                        editor.editorZoom = 8;
                    break;

                default:
                    break;
                }
            }
            drawPaletteWindow();
            drawTilesetWindow();
            drawLevelWindow();
        }

        if(menuState.openSettings)
        {
            ImGui::Begin("Settings", &open);
            ImGui::Text("Preferences go here...");
            if (ImGui::Button("Close"))
                menuState.openSettings = false;
            ImGui::End();
        }

        if(menuState.openHeaderWindow)
        {
            drawHeaderWindow();
        }

        if (menuState.importData)
        {
            if (!ImGuiFileDialog::Instance()->IsOpened())
            {
                IGFD::FileDialogConfig config;
                config.countSelectionMax = 0;

                ImGuiFileDialog::Instance()->OpenDialog(
                    "ChooseData",
                    "Select Bin File",
                    "Binary (*.bin){.bin},All files (*.*){.*}",
                    config
                );
            }
            menuState.importData = false;
        }

        if (menuState.exportGraphics)
        {
            saveROMData();
            if(menuState.exportCommonGFX)
                saveBinary(LF_CommonGFX, editor.selectedLevel, editor.mode);
            else
                saveBinary(LF_LevelGFX, editor.selectedLevel, editor.mode);
            menuState.exportGraphics = false;
        }

        if (menuState.exportMetaTiles)
        {
            saveROMData();
            saveBinary(LF_MetaTileData, editor.selectedLevel, editor.mode);
            menuState.exportMetaTiles = false;
        }

        if (menuState.exportMetaTilePal)
        {
            saveROMData();
            saveBinary(LF_MetaTilePaletteData, editor.selectedLevel, editor.mode);
            menuState.exportMetaTilePal = false;
        }

        if (menuState.exportLayout)
        {
            saveROMData();
            saveBinary(LF_LevelData, editor.selectedLevel, editor.mode);
            menuState.exportLayout = false;
        }

        if (menuState.exportScroll)
        {
            saveROMData();
            saveBinary(LF_ScrollData, editor.selectedLevel, editor.mode);
            menuState.exportScroll = false;
        }

        if (menuState.exportEnemy)
        {
            saveROMData();
            saveBinary(LF_EnemyData, editor.selectedLevel, editor.mode);
            menuState.exportEnemy = false;
        }

        if (menuState.exportItem)
        {
            saveROMData();
            saveBinary(LF_ItemData, editor.selectedLevel, editor.mode);
            menuState.exportItem = false;
        }

        if (menuState.exportMidpoint)
        {
            saveROMData();
            saveBinary(LF_MidPointData, editor.selectedLevel, editor.mode);
            menuState.exportMidpoint = false;
        }

        if (menuState.exportPatternTable)
        {
            saveROMData();
            saveBinary(LF_PatternTable, editor.selectedLevel, editor.mode);
            menuState.exportPatternTable = false;
        }

        if (menuState.exportPalette)
        {
            saveROMData();
            saveBinary(LF_PaletteData, editor.selectedLevel, editor.mode);
            menuState.exportPalette = false;
        }

        if (menuState.exportPaletteAnimation)
        {
            saveROMData();
            saveBinary(LF_PaletteAnimationData, editor.selectedLevel, editor.mode);
            menuState.exportPaletteAnimation = false;
        }

        if (menuState.exportAllLevelData)
        {
            static LevelField currentField = LF_CommonGFX;
            if (currentField == LF_CommonGFX)
            {
                saveROMData();
            }
            if (!exportingData)
            {
                currentField = static_cast<LevelField>(static_cast<int>(currentField) + 1);
            }
            saveBinary(currentField, editor.selectedLevel, editor.mode);
            if (currentField >= LF_END)
            {
                menuState.exportAllLevelData = false;
                currentField = LF_CommonGFX;
            }
        }

        ImGui::Render();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        ImGuiIO& io = ImGui::GetIO();
        if(io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            SDL_Window* backup_window = SDL_GL_GetCurrentWindow();
            SDL_GLContext backup_context = SDL_GL_GetCurrentContext();

            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();

            SDL_GL_MakeCurrent(backup_window, backup_context);
        }

        SDL_GL_SwapWindow(window);
    }
}

void App::saveBinary(const LevelField field, const int lvl, const int mode)
{
    if (ImGuiFileDialog::Instance()->IsOpened())
        return;

    auto& names = editor.data[mode].levelNames;
    std::string levelName = names[lvl];
    const LevelEntry& level = editor.data[mode].levels.at(levelName);

    uint32_t offset = 0;
    uint32_t size = 0;

    switch (field)
    {
        default:
            return;
        case LF_CommonGFX:
            levelName = "common";
            [[fallthrough]];
        case LF_LevelGFX: {
            const auto& gfx = editor.data[mode].gfx;
            auto& levelGfx = gfx.at(levelName);

            if (currentExportIndex == -1)
            {
                exportGFX = true;
                currentExportIndex = 0;
            }

            if (currentExportIndex >= levelGfx.ranges.size())
            {
                exportGFX = false;
                menuState.exportCommonGFX = false;
                currentExportIndex = -1;
                exportingData = false;
                return;
            }

            uint32_t start = levelGfx.ranges[currentExportIndex].start;
            uint32_t end = levelGfx.ranges[currentExportIndex].end;

            offset = start;
            size = end - start;
            break;
        }
        case LF_MetaTileData:
            offset = level.chip32x32;
            size = 0x400;
            break;
        case LF_MetaTilePaletteData:
            offset = level.chip32x32_palette;
            size = 0x100;
            break;
        case LF_LevelData:
            offset = level.map;
            size = 0x2F00;
            break;
        case LF_ScrollData:
            offset = level.scroll;
            size = 0x200;
            break;
        case LF_EnemyData:
            offset = level.enemy_screen;
            size = 0x300;
            break;
        case LF_ItemData:
            offset = level.item_screen;
            size = 0xC0;
            break;
        case LF_MidPointData:
            offset = level.midpoint_start_y;
            size = 0x3C;
            break;
        case LF_PatternTable:
            offset = level.pattern;
            size = 0x200;
            break;
        case LF_PaletteData:
            offset = level.palette_data;
            size = mode == 0 ? 16 : 256;
            break;
        case LF_PaletteAnimationData: {
            if (currentExportIndex == -1)
            {
                offset = level.palette_afc;
                size = 2;
                --currentExportIndex;
                break;
            }
            else if (currentExportIndex == -2)
            {
                offset = level.palette_anime;
                if (mode == 0)
                {
                    uint8_t fc = editor.rom[level.palette_afc];
                    currentExportIndex = -1;
                    if (fc == 0)
                        return;
                    size = fc * 16;
                }
                else
                {
                    size = std::popcount(editor.rom[level.palette_anime]) * 2;
                    --currentExportIndex;
                }
                break;
            }
            else
            {
                uint8_t frameCount = editor.rom[level.palette_afc];
                uint8_t yy = editor.rom[level.palette_anime];
                int numBlocks = std::popcount(yy);

                if (currentExportIndex == -3)
                    currentExportIndex = 0;

                if (currentExportIndex >= frameCount)
                {
                    currentExportIndex = -1;
                    exportingData = false;
                    return;
                }

                int frame = currentExportIndex;

                uint32_t ptr = level.palette_anime + 1 + frame * 2;
                uint16_t src = editor.rom[ptr] | (editor.rom[ptr + 1] << 8);

                if (src == 0xFFFF)
                {
                    currentExportIndex++;
                    return;
                }

                uint32_t tableBank = level.palette_anime & 0xFF0000;

                exportData.clear();
                exportData.reserve(numBlocks * 32);

                for (int bit = 0; bit < 8; ++bit)
                {
                    if (!(yy & (1 << bit)))
                        continue;

                    uint32_t snesAddr = tableBank | (src + bit * 32);
                    if (offset <= 0)
                        offset = snesAddr;
                    uint32_t pcAddr = snesToPc(snesAddr, editor.isHiROM);

                    for (int i = 0; i < 32; ++i)
                        exportData.push_back(editor.rom[pcAddr + i]);
                }
            }
            break;
        }
    }

    if (offset + size > editor.rom.size())
    {
        exportingData = false;
        return;
    }
    else if (size > 0)
    {
        exportData = std::vector<uint8_t>(editor.rom.begin() + offset, editor.rom.begin() + offset + size);
    }
    std::stringstream ss;
    ss << std::uppercase << std::setw(6) << std::setfill('0') << std::hex << offset;
    std::string fileName = levelName + '_' + ss.str() + ".bin";

    exportingData = true;

    IGFD::FileDialogConfig config;
    config.countSelectionMax = 1;
    config.fileName = fileName;
    config.flags = ImGuiFileDialogFlags_ConfirmOverwrite;

    ImGuiFileDialog::Instance()->OpenDialog(
        "SaveBinary",
        "Save Binary",
        "Binary (*.bin){.bin},All files (*.*){.*}",
        config
    );
}

inline ImVec4 ColorU32ToVec4(ImU32 c)
{
    float r = ((c >> IM_COL32_R_SHIFT) & 0xFF) / 255.0f;
    float g = ((c >> IM_COL32_G_SHIFT) & 0xFF) / 255.0f;
    float b = ((c >> IM_COL32_B_SHIFT) & 0xFF) / 255.0f;
    float a = ((c >> IM_COL32_A_SHIFT) & 0xFF) / 255.0f;
    return ImVec4(r, g, b, a);
}

void App::drawTilesetWindow()
{
    ImGui::Begin("Tileset", &open, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::Text("CONTROLS:");
    ImGui::Text("CTRL+= and CTRL+- Zooms in and out");
    if (editor.tileViewMode == VM_Tileset)
        ImGui::Text("Left Click to Select a Tile");
    else if (editor.tileViewMode == VM_Metatiles)
        ImGui::Text("Left Click to Select a Meta Tile");
    else
        ImGui::Text("Left Click to Select a Collision Type");
    ImGui::Separator();

    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootWindow))
    {
        editor.activeWindow = AW_Tileset;
    }

    if (ImGui::BeginCombo("Modes", editor.data[editor.mode].modeName.c_str()))
    {
        for (size_t i = 0; i < editor.data.size(); ++i)
        {
            bool selected = (i == editor.mode);
            if (ImGui::Selectable(editor.data[i].modeName.c_str(), selected))
            {
                saveROMData();
                editor.rebuildData = true;
                editor.mode = i;
                drawPaletteWindow();
            }
            if (selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    auto& names = editor.data[editor.mode].levelNames;

    if (ImGui::BeginCombo("Level", names[editor.selectedLevel].c_str()))
    {
        for (size_t i = 0; i < names.size(); ++i)
        {
            bool selected = (i == editor.selectedLevel);
            if (ImGui::Selectable(names[i].c_str(), selected))
            {
                saveROMData();
                editor.rebuildData = true;
                editor.selectedLevel = i;
                drawPaletteWindow();
            }
            if (selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    const std::string& levelName = names[editor.selectedLevel];
    const LevelEntry& level = editor.data[editor.mode].levels.at(levelName);
    const auto& gfx = editor.data[editor.mode].gfx;
    auto levelGfx = gfx.at(levelName);

    ImGui::Text("Tileset for %s", levelName.c_str());

    int common = 0;

    if (gfx.find("common") != gfx.end())
    {
        const auto& commonGfx = gfx.at("common");

        if (levelGfx.commonIdx.empty())
        {
            levelGfx.ranges.insert(
                levelGfx.ranges.begin(),
                commonGfx.ranges.begin(),
                commonGfx.ranges.end()
            );
            for (const auto& g : commonGfx.ranges)
            {
                //ImGui::Text("Common GFX Range: %06X - %06X", g.start, g.end);
                ++common;
            }
        }
        else
        {
            for (auto it = levelGfx.commonIdx.rbegin(); it != levelGfx.commonIdx.rend(); ++it)
            {
                size_t i = *it;
                if (i >= commonGfx.ranges.size())
                    throw std::runtime_error("commonIdx index out of range");

                levelGfx.ranges.insert(levelGfx.ranges.begin(), commonGfx.ranges[i]);

                //ImGui::Text("Common GFX Range: %06X - %06X", commonGfx.ranges[i].start, commonGfx.ranges[i].end);
                ++common;
            }
        }
    }

    if (editor.rebuildData)
    {
        editor.rebuildData = false;

        editor.rebuildView = true;
        editor.rebuildTileset = true;

        editor.levelTiles = decodeTileRanges(levelGfx.ranges, editor.rom);

        editor.levelTileMap = makeTileMap(editor.levelTiles, 16, 1);

        auto levelMetaTileData = decodeMetaTile32(editor.rom, level.chip32x32, 0x400);
        loadMetaTilePalettes(levelMetaTileData, editor.rom, level.chip32x32_palette, 0x100);

        editor.levelMacroTiles = buildMacroTiles(editor.levelTileMap);
        editor.levelMetaTiles = makeMetaTiles(levelMetaTileData, editor.levelMacroTiles);

        editor.levelData = loadLevelData(editor.rom, level.map, 0x2F00);
        editor.scrollData = loadScrollData(editor.rom, level.scroll, 0x200);
        editor.levelData = remapColumnMajorScreensHorizontally(editor.levelData);

        editor.enemyData = loadObjectData(editor.rom, level.enemy_screen, level.enemy_x, level.enemy_y, level.enemy_type, 0x100);
        editor.itemData = loadObjectData(editor.rom, level.item_screen, level.item_x, level.item_y, level.item_type, 0x40);

        if (editor.mode)
        {
            editor.palettes = decodeCGRAMPalettes(editor.rom, level.palette_data, 16);
            editor.animate = loadAnimatedPalettes(editor.rom, level.palette_afc, level.palette_aft, level.palette_anime, editor.isHiROM, editor.palettes);
        }
        else
        {
            editor.palettes = makeNESPalettes(editor.rom, level.palette_data, editor.nesMasterPalette);
            editor.animate = loadAnimatedPalettesNES(editor.rom, level.palette_afc, level.palette_aft, level.palette_anime, editor.nesMasterPalette);
        }

        editor.aniPalettes = editor.palettes;
        editor.paletteIndex = 0;
        editor.aniPalIndex = 0;
    }

    /*for (size_t i = common; i < levelGfx.ranges.size(); ++i)
        ImGui::Text("GFX Range: %06X - %06X", levelGfx.ranges[i].start, levelGfx.ranges[i].end);

    ImGui::Text("chip32x32: %06X", level.chip32x32);
    ImGui::Text("palette:   %06X", level.chip32x32_palette);
    ImGui::Text("pattern:   %06X", level.pattern);*/

    static bool force = false;
    const char* tabNames[] = { "Tileset", "Meta Tiles", "Collision" };
    if (ImGui::BeginCombo("Tile Mode", tabNames[editor.tileViewMode]))
    {
        for (int i = 0; i < 3; ++i)
        {
            bool selected = (i == editor.tileViewMode);
            if (ImGui::Selectable(tabNames[i], selected))
            {
                editor.tileViewMode = static_cast<ViewMode>(i);
                editor.selectedTile = -1;
                editor.rebuildView = true;
                editor.rebuildTileset = true;
            }
            if (selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    
    if (editor.tileViewMode != VM_Collision)
    {
        ImGui::Text("Selected Tile: %d", editor.selectedTile);
        drawTileView(editor.tileset, editor.tileViewMode, editor.tilesetZoom);
    }
    else
    {
        ImGui::SeparatorText("Collision Types");

        for (auto& ct : collisionTypes)
        {
            ImGui::PushID(ct.id);

            ImVec4 col = ColorU32ToVec4(ct.color);
            ImGui::ColorButton("##col", col, 0, ImVec2(20, 20));
            ImGui::SameLine();

            if (ImGui::Selectable(ct.name, editor.selectedTile == ct.id))
                editor.selectedTile = ct.id;

            ImGui::PopID();
        }
    }

    ImGui::End();
}

void App::applyAnimationFrame()
{
    if (editor.animate.frames.empty()) return;

    uint8_t amount = editor.mode == 0 ? 4 : 6;
    uint8_t offset = editor.mode == 0 ? 0 : 2;
    std::copy_n(editor.animate.frames.begin() + (editor.animFrame * amount), amount, editor.aniPalettes.begin() + offset);
}

void App::updatePaletteAnimation()
{
    if (editor.animate.frame_count == 0 || editor.animate.frame_timer == 0 || !editor.animatePalettes)
        return;

    if (--editor.animTimer <= 0)
    {
        editor.animTimer = editor.animate.frame_timer;
        editor.animFrame++;

        if (editor.animFrame >= editor.animate.frame_count)
            editor.animFrame = 0;

        applyAnimationFrame();

        editor.rebuildTileset = true;
        editor.rebuildView = true;
    }
}

void App::DrawColorButton(const std::string& id, ColorRGBA& col, bool isAni, size_t paletteIndex, int colorIndex, const char* popupName, const LevelEntry& level, ImVec2 size)
{
    ImVec4 c(col.r / 255.f, col.g / 255.f, col.b / 255.f, 1.f);

    if (ImGui::ColorButton(id.c_str(), c, 0, size))
    {
        editor.editingPalette = paletteIndex;
        editor.editingColor = colorIndex;
        editor.editingAniPal = isAni;
        editor.tempColor = col;
        ImGui::OpenPopup(popupName);
    }

    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::MenuItem("Copy Color"))
        {
            editor.colClip.color = col;
            editor.colClip.hasData = true;
        }

        if (ImGui::MenuItem("Paste Color", nullptr, false, editor.colClip.hasData))
        {
            editor.editingPalette = paletteIndex;
            editor.editingColor = colorIndex;
            editor.tempColor = editor.colClip.color;
            editor.editingAniPal = isAni;
            if (editor.mode == 0)
                writeNESColorToROM(level, -1);
            else
                writeSNESColorToROM(level);
            
        }

        if (ImGui::MenuItem("Swap With Clipboard", nullptr, false, editor.colClip.hasData))
        {
            std::swap(col, editor.colClip.color);
            editor.editingPalette = paletteIndex;
            editor.editingColor = colorIndex;
            editor.tempColor = col;
            editor.editingAniPal = isAni;
            if (editor.mode == 0)
                writeNESColorToROM(level, -1);
            else
                writeSNESColorToROM(level);
        }

        ImGui::EndPopup();
    }

    ImGui::SameLine();
}

void App::DrawPaletteRow(const char* label, size_t index, Palette& pal, int colorsPerPalette, bool isAni, const char* popupName, const char* ident, const LevelEntry& level)
{
    std::string id = label;
    id += " " + std::to_string(index);
    std::string popupId = id + "_popup";

    if (ImGui::Button(id.c_str()))
        ImGui::OpenPopup(popupId.c_str());

    if (ImGui::BeginPopup(popupId.c_str()))
    {
        if (ImGui::MenuItem("Copy Palette"))
        {
            editor.palClip.colors = pal;
            editor.palClip.hasData = true;
        }

        if (ImGui::MenuItem("Paste Palette", nullptr, false, editor.palClip.hasData))
        {
            pal = editor.palClip.colors;
            if (editor.mode == 0)
                writeNESPaletteToROM(index, pal, isAni, level);
            else
                writeSNESPaletteToROM(index, pal, isAni, level);
        }

        if (ImGui::MenuItem("Swap With Clipboard", nullptr, false, editor.palClip.hasData))
        {
            std::swap(pal, editor.palClip.colors);
            if (editor.mode == 0)
                writeNESPaletteToROM(index, pal, isAni, level);
            else
                writeSNESPaletteToROM(index, pal, isAni, level);
        }

        ImGui::EndPopup();
    }

    for (int i = 0; i < colorsPerPalette; ++i)
    {
        id = std::string(ident) + "_" + std::to_string(index) + "_" + std::to_string(i);

        DrawColorButton(id, pal[i], isAni, index, i, popupName, level, colorsPerPalette == 16 ? ImVec2(20, 20) : ImVec2(0, 0));
    }

    ImGui::NewLine();
}

void App::DrawAllPalettes(int colorsPerPalette, const char* popupName, const LevelEntry& level)
{
    size_t half = editor.mode == 0 ? (editor.palettes.size() / 4) : (editor.palettes.size() / 2);

    for (size_t p = 0; p < half * 2; ++p)
    {
        const char* label;
        const char* id;

        if (p < 2)
        {
            label = "SP Palette";
            id = "SP";
        }
        else if (p < half)
        {
            label = "BG Palette";
            id = "BG";
        }
        else
        {
            label = "SPR Palette";
            id = "SPR";
        }

        size_t index = (p < half) ? p : p - half;

        DrawPaletteRow(label, index, editor.palettes[p], colorsPerPalette, false, popupName, id, level);
    }
}

void App::DrawAnimatedPalettes(int colorsPerPalette, int colorsPerFrame, const char* popupName, const LevelEntry& level)
{
    int frameCount = editor.animate.frame_count;
    if (ImGui::InputInt("Animation Frame Count", &frameCount))
    {
        frameCount = std::clamp(frameCount, 0, 4);
        editor.animate.frame_count = frameCount;
        editor.rom[level.palette_afc] = frameCount;
        editor.animate = loadAnimatedPalettes(editor.rom, level.palette_afc, level.palette_aft, level.palette_anime, editor.isHiROM, editor.palettes);
    }

    if (frameCount == 0)
        return;

    int frameTimer = editor.animate.frame_timer;
    if (ImGui::InputInt("Animation Frame Timer", &frameTimer))
    {
        editor.animate.frame_timer = std::clamp(frameTimer, 0, 255);
        editor.rom[level.palette_aft] = editor.animate.frame_timer;
    }

    bool changed = false;
    if (ImGui::BeginCombo("Animation Frame", std::to_string(editor.aniPalIndex).c_str()))
    {
        for (int i = 0; i < frameCount; ++i)
        {
            bool selected = (i == editor.aniPalIndex);
            if (ImGui::Selectable(std::to_string(i).c_str(), selected))
            {
                editor.aniPalIndex = i;
                changed = true;
            }
            if (selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    bool checked = ImGui::Checkbox("Animate Palette", &editor.animatePalettes);

    if ((checked || changed) && !editor.animatePalettes)
    {
        editor.animFrame = editor.aniPalIndex;
        applyAnimationFrame();
        editor.rebuildTileset = true;
        editor.rebuildView = true;
    }

    size_t start = editor.aniPalIndex * colorsPerFrame;
    size_t end = start + colorsPerFrame;

    for (size_t p = start; p < end && p < editor.animate.frames.size(); ++p)
        DrawPaletteRow("ANI Palette", p, editor.animate.frames[p], colorsPerPalette, true, popupName, "ANI", level);
}

void App::DrawNESPopup(const LevelEntry& level)
{
    if (!ImGui::BeginPopup("Pick NES Color"))
        return;

    ImGui::Text("Select NES Color");

    for (int i = 0; i < 64; ++i)
    {
        const auto& c = editor.nesMasterPalette[i];
        ImVec4 col(c.r / 255.f, c.g / 255.f, c.b / 255.f, 1.f);

        std::string id = "nescol_" + std::to_string(i);

        if (ImGui::ColorButton(id.c_str(), col, 0, ImVec2(20, 20)))
        {
            writeNESColorToROM(level, i);
            ImGui::CloseCurrentPopup();
        }

        if ((i % 16) != 15)
            ImGui::SameLine();
    }

    ImGui::EndPopup();
}

void App::DrawSNESPopup(const LevelEntry& level)
{
    if (!ImGui::BeginPopup("Edit SNES Color"))
        return;

    ImGui::Text("Edit SNES 15-bit Color");

    ImVec4 col(editor.tempColor.r / 255.f,
        editor.tempColor.g / 255.f,
        editor.tempColor.b / 255.f, 1.f);

    if (ImGui::ColorPicker3("Color", (float*)&col))
    {
        editor.tempColor = {
            uint8_t(col.x * 255),
            uint8_t(col.y * 255),
            uint8_t(col.z * 255)
        };
    }

    if (ImGui::Button("Apply"))
    {
        writeSNESColorToROM(level);
        ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();

    if (ImGui::Button("Cancel"))
        ImGui::CloseCurrentPopup();

    ImGui::EndPopup();
}

int App::findNESIndexFromRGBA(const ColorRGBA& col)
{
    int best = 0;
    int bestDist = INT_MAX;

    for (int i = 0; i < 64; ++i)
    {
        const auto& m = editor.nesMasterPalette[i];
        int dr = int(m.r) - col.r;
        int dg = int(m.g) - col.g;
        int db = int(m.b) - col.b;
        int dist = dr * dr + dg * dg + db * db;

        if (dist < bestDist)
        {
            bestDist = dist;
            best = i;
        }
    }

    return best;
}

void App::writeNESColorToROM(const LevelEntry& level, int index)
{
    if (index < 0)
        index = findNESIndexFromRGBA(editor.tempColor);

    uint32_t base = level.palette_data;
    if (editor.editingAniPal)
        base = level.palette_anime;

    uint32_t addr = base + editor.editingPalette * 4 + editor.editingColor;

    editor.rom[addr] = index;

    if (!editor.editingAniPal)
    {
        editor.palettes = makeNESPalettes(editor.rom, base, editor.nesMasterPalette);
        editor.aniPalettes = editor.palettes;
    }
    else
    {
        editor.animate = loadAnimatedPalettesNES(editor.rom, level.palette_afc, level.palette_aft, level.palette_anime, editor.nesMasterPalette);
    }

    applyAnimationFrame();

    editor.rebuildTileset = true;
    editor.rebuildView = true;
}

void App::writeSNESColorToROM(const LevelEntry& level)
{
    if (editor.editingAniPal)
    {
        editor.animate.frames[editor.editingPalette][editor.editingColor] = editor.tempColor;
        writeAnimatedPalettes(editor.rom, level.palette_anime, editor.isHiROM, editor.animate);
        editor.animate = loadAnimatedPalettes(editor.rom, level.palette_afc, level.palette_aft, level.palette_anime, editor.isHiROM, editor.palettes);
    }
    else
    {
        uint32_t base = level.palette_data;
        uint32_t addr = base + (editor.editingPalette * 16 + editor.editingColor) * 2;

        writeSNESColor(editor.rom, addr, editor.tempColor);

        editor.palettes = decodeCGRAMPalettes(editor.rom, base, 16);
        editor.aniPalettes = editor.palettes;
    }

    applyAnimationFrame();

    editor.rebuildTileset = true;
    editor.rebuildView = true;
}

void App::writeNESPaletteToROM(size_t paletteIndex, const Palette& pal, const bool isAni, const LevelEntry& level)
{
    uint32_t base = level.palette_data;
    if (isAni)
        base = level.palette_anime;

    for (int i = 0; i < 4; ++i)
    {
        int nesIndex = findNESIndexFromRGBA(pal[i]);
        uint32_t addr = base + paletteIndex * 4 + i;
        editor.rom[addr] = uint8_t(nesIndex);
    }

    editor.palettes = makeNESPalettes(editor.rom, base, editor.nesMasterPalette);
    editor.aniPalettes = editor.palettes;

    applyAnimationFrame();

    editor.rebuildTileset = true;
    editor.rebuildView = true;
}

void App::writeSNESPaletteToROM(size_t paletteIndex, const Palette& pal, const bool isAni, const LevelEntry& level)
{
    if (isAni)
    {
        editor.animate.frames[paletteIndex] = pal;
        writeAnimatedPalettes(editor.rom, level.palette_anime, editor.isHiROM, editor.animate);
        editor.animate = loadAnimatedPalettes(editor.rom, level.palette_afc, level.palette_aft, level.palette_anime, editor.isHiROM, editor.palettes);
    }
    else
    {
        uint32_t base = level.palette_data;

        for (int i = 0; i < 16; ++i)
        {
            uint32_t addr = base + (paletteIndex * 16 + i) * 2;
            writeSNESColor(editor.rom, addr, pal[i]);
        }
        editor.palettes = decodeCGRAMPalettes(editor.rom, base, 16);
        editor.aniPalettes = editor.palettes;
    }

    applyAnimationFrame();

    editor.rebuildTileset = true;
    editor.rebuildView = true;
}

void App::drawPaletteWindow()
{
    ImGui::Begin("Palettes", &open, ImGuiWindowFlags_HorizontalScrollbar);
    
    ImGui::Text("CONTROLS:");
    ImGui::Text("Left Click a color to change it");
    ImGui::Text("Left Click a palette name to copy, paste, or swap with clipboard");
    ImGui::Separator();

    auto& names = editor.data[editor.mode].levelNames;
    const std::string& levelName = names[editor.selectedLevel];
    const LevelEntry& level = editor.data[editor.mode].levels.at(levelName);

    ImGui::Text("Palettes for %s", levelName.c_str());

    bool isNES = (editor.mode == 0);

    uint8_t offset = isNES ? 0 : 2;
    int colorsPerFrame = isNES ? 4 : 6;
    if (editor.paletteIndex < offset)
        editor.paletteIndex = offset;

    if(ImGui::BeginCombo("BG Palette Index", std::to_string(editor.paletteIndex).c_str()))
    {
        for(size_t i = offset; i < colorsPerFrame; ++i)
        {
            bool selected = (i == editor.paletteIndex);

            if(ImGui::Selectable(std::to_string(i).c_str(), selected))
            {
                editor.paletteIndex = i;
                editor.rebuildTileset = true;
            }
            if(selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    if (ImGui::Checkbox("Universal BG Color", &editor.univeralBGColor))
    {
        editor.rebuildTileset = true;
        editor.rebuildView = true;
    }

    int colorsPerPalette = isNES ? 4 : 16;
    
    const char* popup = isNES ? "Pick NES Color" : "Edit SNES Color";

    DrawAllPalettes(colorsPerPalette, popup, level);
    DrawAnimatedPalettes(colorsPerPalette, colorsPerFrame, popup, level);

    if (isNES)
        DrawNESPopup(level);
    else
        DrawSNESPopup(level);

    ImGui::End();
}

inline void DrawTextOutlined(ImDrawList* dl, ImVec2 pos, ImU32 colText, const char* text)
{
    ImU32 colOutline = IM_COL32(255, 255, 255, 255);

    dl->AddText(ImVec2(pos.x - 1, pos.y), colOutline, text);
    dl->AddText(ImVec2(pos.x + 1, pos.y), colOutline, text);
    dl->AddText(ImVec2(pos.x, pos.y - 1), colOutline, text);
    dl->AddText(ImVec2(pos.x, pos.y + 1), colOutline, text);

    dl->AddText(pos, colText, text);
}

void App::drawLevelWindow()
{
    ImGui::Begin("Editor View", &open, ImGuiWindowFlags_HorizontalScrollbar);

    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootWindow))
    {
        editor.activeWindow = AW_Editor;
    }

    ViewMode view = VM_Invalid;

    switch (editor.tileViewMode)
    {
    case VM_Collision:
        view = VM_Collision;
        [[fallthrough]];
    case VM_Tileset:
    {
        static TilemapTexture meta;
        ImGui::Text("CONTROLS:");
        ImGui::Text("CTRL+= and CTRL+- Zooms in and out");
        if (view == VM_Collision)
        {
            ImGui::Text("Hold Left Click to Paint with the Selected Collision Type");
        }
        else
        {
            ImGui::Text("Hold Left Click to Paint with the Selected Tile");
            ImGui::Text("The selected Tile will use the currently Select Palette");
            ImGui::Text("Right Click to Paint with the currently Selected Palette");
        }
        ImGui::Separator();
        drawTileView(meta, view, editor.editorZoom);
        break;
    }
    default:
        drawLevelView();
        break;
    }

    ImGui::End();
}

void App::drawHeaderWindow()
{
    ImGui::Begin("SNES Header Info", &menuState.openHeaderWindow);

    ImGui::Text("Title: %s", editor.header.title);
    ImGui::Separator();

    ImGui::Text("Map Mode:        0x%02X", editor.header.mapMode);
    ImGui::Text("ROM Type:        0x%02X", editor.header.romType);
    ImGui::Text("ROM Size:        0x%02X (%d KB)", editor.header.romSize, 1 << (editor.header.romSize));
    ImGui::Text("SRAM Size:       0x%02X", editor.header.sramSize);
    ImGui::Text("Country:         0x%02X", editor.header.country);
    ImGui::Text("License:         0x%02X", editor.header.license);
    ImGui::Text("Version:         0x%02X", editor.header.version);

    ImGui::Separator();

    ImGui::Text("Checksum:        0x%04X", editor.header.checksum);
    ImGui::Text("Checksum Comp:   0x%04X", editor.header.checksumComp);

    ImGui::Separator();

    ImGui::Text("Detected Mapping: %s", editor.isHiROM ? "HiROM" : "LoROM");

    ImGui::End();
}

inline void DrawNearestImage(ImDrawList* dl, const TilemapTexture& tileset, const ImVec2& size, const ImVec2 c1, const ImVec2 c2)
{
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();

    dl->AddCallback(platform_io.DrawCallback_SetSamplerNearest, nullptr);

    ImGui::Image((ImTextureID)(intptr_t)tileset.tex, size, c1, c2);

    dl->AddCallback(platform_io.DrawCallback_SetSamplerLinear, nullptr);
}

inline bool GetTileUnderMouse(const ImVec2& min, int s, int& outX, int& outY)
{
    ImVec2 mouse = ImGui::GetMousePos();
    ImVec2 rel(mouse.x - min.x, mouse.y - min.y);

    outX = (int)(rel.x / s);
    outY = (int)(rel.y / s);

    return true;
}

inline void DrawGrid(ImDrawList* dl, const ImVec2& min, const ImVec2& max, int s, ImU32 color = IM_COL32(80, 80, 80, 128))
{
    int cols = (max.x - min.x) / s;
    int rows = (max.y - min.y) / s;

    for (int x = 0; x <= cols; ++x)
        dl->AddLine(ImVec2(min.x + x * s, min.y), ImVec2(min.x + x * s, max.y), color);

    for (int y = 0; y <= rows; ++y)
        dl->AddLine(ImVec2(min.x, min.y + y * s), ImVec2(max.x, min.y + y * s), color);
}

inline void DrawHoverHighlight(ImDrawList* dl, const ImVec2& min, int s, int x, int y)
{
    float x0 = min.x + x * s;
    float y0 = min.y + y * s;
    float x1 = x0 + s;
    float y1 = y0 + s;

    dl->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y1), IM_COL32(100, 150, 255, 60));
    dl->AddRect(ImVec2(x0, y0), ImVec2(x1, y1), IM_COL32(100, 150, 255, 255), 0, 0, 2.0f);
}

inline void DrawSelectedOutline(ImDrawList* dl, const ImVec2& min, int s, int x, int y)
{
    float x0 = min.x + x * s;
    float y0 = min.y + y * s;
    float x1 = x0 + s;
    float y1 = y0 + s;

    dl->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y1), IM_COL32(255, 255, 0, 40));
    dl->AddRect(ImVec2(x0, y0), ImVec2(x1, y1), IM_COL32(255, 255, 0, 255), 0, 0, 2.0f);
}

inline void DrawTilePreview(
    ImDrawList* dl,
    const ImVec2& min,
    int ts,
    int s,
    int tileX, int tileY,
    int atlasX, int atlasY,
    const TilemapTexture& tileset)
{
    float x0 = min.x + tileX * ts;
    float y0 = min.y + tileY * ts;
    float x1 = x0 + ts;
    float y1 = y0 + ts;

    float u0 = (atlasX * s) / (float)tileset.width;
    float v0 = (atlasY * s) / (float)tileset.height;
    float u1 = ((atlasX + 1) * s) / (float)tileset.width;
    float v1 = ((atlasY + 1) * s) / (float)tileset.height;

    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();

    dl->AddCallback(platform_io.DrawCallback_SetSamplerNearest, nullptr);

    dl->AddImage(
        (ImTextureID)(intptr_t)tileset.tex,
        ImVec2(x0, y0), ImVec2(x1, y1),
        ImVec2(u0, v0), ImVec2(u1, v1),
        IM_COL32(255, 255, 255, 200)
    );

    dl->AddCallback(platform_io.DrawCallback_SetSamplerLinear, nullptr);

    dl->AddRect(ImVec2(x0, y0), ImVec2(x1, y1), IM_COL32(255, 255, 0, 255), 0, 0, 2.0f);
}

inline void App::SelectTileFromClick(int tileX, int tileY, int atlasWidth)
{
    editor.selectedTile = tileY * atlasWidth + tileX;
}

inline void App::PaintTileGeneric(int tileX, int tileY, int atlasWidth, const bool color)
{
    if (tileX < 0 || tileY < 0)
        return;

    int index = tileY * atlasWidth + tileX;

    const int tilesPerRow = 32;
    const int metaTilesPerRow = tilesPerRow / 2;

    int metaX = tileX / 2;
    int metaY = tileY / 2;
    int metaTileIndex = metaY * metaTilesPerRow + metaX;

    int localX = tileX % 2;
    int localY = tileY % 2;
    int localIndex = localY * 2 + localX;

    if (metaTileIndex < 0 || metaTileIndex >= editor.levelMetaTiles.size())
        return;

    if (!color)
    {
        editor.levelMetaTiles[metaTileIndex].tiles[localIndex].left = editor.levelMacroTiles[editor.selectedTile].left;

        editor.levelMetaTiles[metaTileIndex].tiles[localIndex].right = editor.levelMacroTiles[editor.selectedTile].right;

        editor.levelMetaTiles[metaTileIndex].macroIndex[localIndex] = editor.selectedTile;
    }

    uint8_t offset = editor.mode == 0 ? 0 : 2;
    editor.levelMetaTiles[metaTileIndex].palettes[localIndex] = editor.paletteIndex - offset;

    editor.rebuildTileset = true;
}

inline void DrawCollisionBox(ImDrawList* dl, const ImVec2& min, int s, int x, int y, ImU32 color)
{
    float x0 = min.x + x * s;
    float y0 = min.y + y * s;
    float x1 = x0 + s;
    float y1 = y0 + s;

    dl->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y1), color);
    dl->AddRect(ImVec2(x0, y0), ImVec2(x1, y1), IM_COL32(0, 0, 0, 180), 0, 0, 1.0f);
}

inline void DrawCollisionPreview(ImDrawList* dl, const ImVec2& min, int s, int x, int y, ImU32 color)
{
    float x0 = min.x + x * s;
    float y0 = min.y + y * s;
    float x1 = x0 + s;
    float y1 = y0 + s;

    dl->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y1), color);
    dl->AddRect(ImVec2(x0, y0), ImVec2(x1, y1), IM_COL32(255, 255, 255, 255), 0, 0, 2.0f);
}

void App::PaintTilePixel(int macroIndex, int x, int y)
{
    if (macroIndex < 0 || macroIndex >= editor.levelMacroTiles.size())
        return;

    MacroTile& mt = editor.levelMacroTiles[macroIndex];

    TileRef& tr = x > 7 ? mt.right : mt.left;
    
    int tileIndex = y > 7 ? tr.index + 1 : tr.index;

    Tile& t = editor.levelTiles[tileIndex];

    y = y % 8;
    x = x % 8;

    t.pixels[y * 8 + x] = editor.selectedColor;

    saveTileToROM(t, editor.rom);

    editor.rebuildTileset = true;
}

void App::drawTileView(TilemapTexture& tileset, const ViewMode view, const int zoom)
{
    int tileSize = (view == VM_Metatiles ? 32 : 16);
    float scale = (view == VM_Tileset ? 2.0f : 1.0f) * zoom;

    int trueSize = tileSize * scale;
    static bool wait = false;

    if (editor.rebuildTileset || tileset.tex == 0)
    {
        if (tileset.tex != 0)
            glDeleteTextures(1, &tileset.tex);

        std::vector<ColorRGBA> outPixels;

        ColorRGBA bgColor = editor.palettes[0][0];
        if (!editor.univeralBGColor)
            bgColor.a = 0;

        switch (view)
        {
        case VM_Tileset:
            renderTileMapToRGBA(editor.levelTileMap, editor.levelTiles, editor.aniPalettes[editor.paletteIndex], bgColor, outPixels, tileset.width, tileset.height);
            wait = false;
            break;
        default:
            uint8_t offset = editor.mode == 0 ? 0 : 2;
            renderMetaTileMapToRGBA(editor.levelMetaTiles, 16, editor.levelTiles, editor.aniPalettes, offset, bgColor, outPixels, tileset.width, tileset.height);
            if (!wait)
                editor.rebuildTileset = false;
            break;
        }
        
        uploadTilemapTextureRGBA(outPixels, tileset);
    }
    
    ImDrawList* dl = ImGui::GetWindowDrawList();

    int trueWidth = tileset.width * scale;
    int trueHeight = tileset.height * scale;

    DrawNearestImage(dl, tileset, ImVec2(trueWidth, trueHeight), ImVec2(0, 0), ImVec2(1, 1));

    ImVec2 min = ImGui::GetItemRectMin();
    ImVec2 max = ImGui::GetItemRectMax();

    int atlasWidth = trueWidth / trueSize;
    int atlasHeight = trueHeight / trueSize;

    int tileX, tileY;
    GetTileUnderMouse(min, trueSize, tileX, tileY);

    bool hovering = ImGui::IsItemHovered();

    if (view == VM_Collision)
    {
        for (int y = 0; y < atlasHeight; ++y)
        {
            for (int x = 0; x < atlasWidth; ++x)
            {
                int metaX = x / 2;
                int metaY = y / 2;
                int metaIndex = metaY * (atlasWidth / 2) + metaX;

                if (metaIndex < 0 || metaIndex >= editor.levelMetaTiles.size())
                    continue;

                int localX = x % 2;
                int localY = y % 2;
                int localIndex = localY * 2 + localX;

                uint8_t col = editor.levelMetaTiles[metaIndex].collision[localIndex];

                ImU32 color = collisionTypes[col].color;
                if (color != IM_COL32(0, 0, 0, 0))
                    DrawCollisionBox(dl, min, trueSize, x, y, color);
            }
        }

        if(editor.selectedTile >= 0 && hovering)
        {
            ImU32 previewColor = collisionTypes[editor.selectedTile].color;
            DrawCollisionPreview(dl, min, trueSize, tileX, tileY, previewColor);

            if (hovering && ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                int metaX = tileX / 2;
                int metaY = tileY / 2;
                int metaIndex = metaY * (atlasWidth / 2) + metaX;

                if (metaIndex >= 0 && metaIndex < editor.levelMetaTiles.size())
                {
                    int localX = tileX % 2;
                    int localY = tileY % 2;
                    int localIndex = localY * 2 + localX;

                    editor.levelMetaTiles[metaIndex].collision[localIndex] = editor.selectedTile;
                }
            }
        }
    }
    else if (view != VM_Invalid)
    {
        if (hovering)
            DrawHoverHighlight(dl, min, trueSize, tileX, tileY);

        if (ImGui::IsItemClicked())
            SelectTileFromClick(tileX, tileY, atlasWidth);

        if (editor.selectedTile >= 0)
        {
            int selX = editor.selectedTile % atlasWidth;
            int selY = editor.selectedTile / atlasWidth;
            DrawSelectedOutline(dl, min, trueSize, selX, selY);

            if (view == VM_Tileset)
            {
                float u0 = (selX * trueSize) / float(trueWidth);
                float v0 = (selY * trueSize) / float(trueHeight);
                float u1 = ((selX + 1) * trueSize) / float(trueWidth);
                float v1 = ((selY + 1) * trueSize) / float(trueHeight);

                scale = 16.0f * zoom;
                ImVec2 bigSize(tileSize * scale, tileSize * scale);

                ImGui::SameLine();
                ImGui::BeginGroup();
                ImGui::Text("Selected Tile");

                ImDrawList* dl2 = ImGui::GetWindowDrawList();

                DrawNearestImage(dl, tileset, bigSize, ImVec2(u0, v0), ImVec2(u1,v1));

                ImVec2 bigMin = ImGui::GetItemRectMin();
                ImVec2 bigMax = ImGui::GetItemRectMax();

                DrawGrid(dl2, bigMin, bigMax, scale);

                Palette& pal = editor.aniPalettes[editor.paletteIndex];

                for (int i = 0; i < pal.size(); ++i)
                {
                    ImVec4 c(
                        pal[i].r / 255.f,
                        pal[i].g / 255.f,
                        pal[i].b / 255.f,
                        1.f
                    );

                    std::string id = "color_" + std::to_string(i);

                    if (ImGui::ColorButton(id.c_str(), c, 0, ImVec2(20, 20)))
                        editor.selectedColor = i;

                    if (editor.selectedColor == i)
                    {
                        ImDrawList* dl3 = ImGui::GetWindowDrawList();
                        ImVec2 p0 = ImGui::GetItemRectMin();
                        ImVec2 p1 = ImGui::GetItemRectMax();

                        dl3->AddRect(
                            p0, p1,
                            IM_COL32(255, 255, 0, 255),
                            0.0f,
                            0,
                            2.0f
                        );
                    }

                    if ((i % 8) != 7)
                        ImGui::SameLine();
                }
                ImGui::NewLine();

                ImVec2 mouse = ImGui::GetMousePos();
                bool inside =
                    mouse.x >= bigMin.x && mouse.x < bigMax.x &&
                    mouse.y >= bigMin.y && mouse.y < bigMax.y;

                if (inside && ImGui::IsMouseDown(ImGuiMouseButton_Left))
                {
                    int px = int((mouse.x - bigMin.x) / scale);
                    int py = int((mouse.y - bigMin.y) / scale);

                    PaintTilePixel(editor.selectedTile, px, py);
                    wait = true;
                }


                ImGui::EndGroup();
            }
        }
    }
    else if (hovering)
    {
        const int s = 16;
        const int ts = s * editor.editorZoom;

        int atlasW = editor.tileset.width / s;
        int atlasX = editor.selectedTile % atlasW;
        int atlasY = editor.selectedTile / atlasW;

        DrawTilePreview(dl, min, ts, s, tileX, tileY, atlasX, atlasY, editor.tileset);

        if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && editor.selectedTile >= 0)
            PaintTileGeneric(tileX, tileY, atlasWidth, false);

        else if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
            PaintTileGeneric(tileX, tileY, atlasWidth, true);
    }

    DrawGrid(dl, min, max, trueSize);

    if (view == VM_Invalid)
    {
        const int s = 32;
        const int ts = s * editor.editorZoom;
        ImU32 color = IM_COL32(160, 0, 160, 128);
        DrawGrid(dl, min, max, ts, color);
    }
}

void App::drawLevelView()
{
    bool levelMode = (editor.lvlViewMode == 0);
    ImGui::Text("CONTROLS:");
    ImGui::Text("Left and Right Arrow keys Scroll left and right");
    ImGui::Text("CTRL+= and CTRL+- Zooms in and out");
    if (levelMode)
    {
        ImGui::Text("Hold Left Click to Paint with the Selected Meta Tile");
        ImGui::Text("Right Click to grab the currently hovered Meta Tile");
    }
    else
    {
        ImGui::Text("Left Click to Selected an Object");
        ImGui::Text("Hold Left Click to Drag an Object");
    }
    ImGui::Separator();

    auto& names = editor.data[editor.mode].levelNames;
    const std::string& levelName = names[editor.selectedLevel];
    const LevelEntry& level = editor.data[editor.mode].levels.at(levelName);

    /*ImGui::Text("Level: %s", levelName.c_str());
    ImGui::Text("Map address: %06X", level.map);
    ImGui::Text("Scroll: %06X", level.scroll);*/

    const char* cbNames[] = { "Level Editor", "Object Editor" };
    if (ImGui::BeginCombo("Editor Type", cbNames[editor.lvlViewMode]))
    {
        for (int i = 0; i < 2; ++i)
        {
            bool selected = (editor.lvlViewMode == i);
            if (ImGui::Selectable(cbNames[i], selected))
            {
                editor.lvlViewMode = i;
                editor.rebuildView = true;
            }
            if (selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    static int scrollScreen = 0;
    static int currentScreen = -1;
    static TilemapTexture tileGrid;

    size_t si = editor.levelData.size();
    int numScreens = si / 64;
    int fullMetaWidth = numScreens * 8;

    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))  scrollScreen--;
    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) scrollScreen++;

    int maxScroll = std::max(0, numScreens - 2);
    scrollScreen = std::clamp(scrollScreen, 0, maxScroll);

    ImGui::SliderInt("Screens", &scrollScreen, 0, maxScroll);

    if (levelMode)
    {
        if (drawScrollData(editor.scrollData, scrollScreen) && scrollScreen < si - 1)
            drawScrollData(editor.scrollData, scrollScreen + 1);
    }

    int windowX = scrollScreen * 8;

    if (tileGrid.tex == 0 || scrollScreen != currentScreen || editor.rebuildView)
    {
        editor.rebuildView = false;
        currentScreen = scrollScreen;

        std::vector<ColorRGBA> outPixels;

        if (tileGrid.tex != 0)
            glDeleteTextures(1, &tileGrid.tex);

        ColorRGBA bgColor = editor.palettes[0][0];
        if (!editor.univeralBGColor)
            bgColor.a = 0;

        int windowWidth = 16; // 2 screens
        uint8_t offset = editor.mode == 0 ? 0 : 2;
        renderMetaTileWindowToRGBA(
            editor.levelData,
            fullMetaWidth,
            windowX,
            windowWidth,
            editor.levelMetaTiles,
            editor.levelTiles,
            editor.aniPalettes,
            offset,
            bgColor,
            outPixels,
            tileGrid.width,
            tileGrid.height
        );

        uploadTilemapTextureRGBA(outPixels, tileGrid);
    }

    int trueWidth = tileGrid.width * editor.editorZoom;
    int trueHeight = tileGrid.height * editor.editorZoom;

    ImGui::BeginChild("LevelRegion", ImVec2(trueWidth, trueHeight), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    editor.inLevelRegion = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);

    DrawNearestImage(dl, tileGrid, ImVec2(trueWidth, trueHeight), ImVec2(0, 0), ImVec2(1, 1));

    ImVec2 min = ImGui::GetItemRectMin();
    ImVec2 max = ImGui::GetItemRectMax();
    
    bool hovering = ImGui::IsItemHovered();
    const int s = 32;
    const int ts = s * editor.editorZoom;

    ImGui::EndChild();

    if (levelMode)
    {
        DrawGrid(dl, min, max, ts);

        if (editor.inLevelRegion)
        {
            int tileX, tileY;
            GetTileUnderMouse(min, ts, tileX, tileY);

            if (hovering)
            {
                DrawHoverHighlight(dl, min, ts, tileX, tileY);
                if (editor.selectedTile >= 0)
                {
                    int atlasW = editor.tileset.width / s;
                    int atlasX = editor.selectedTile % atlasW;
                    int atlasY = editor.selectedTile / atlasW;

                    DrawTilePreview(dl, min, ts, s, tileX, tileY, atlasX, atlasY, editor.tileset);

                    if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
                    {
                        int worldTileX = tileX + windowX;
                        int worldTileY = tileY;

                        int index = worldTileY * fullMetaWidth + worldTileX;

                        if (index >= 0 && index < editor.levelData.size() && editor.inLevelRegion)
                        {
                            editor.levelData[index] = editor.selectedTile;
                            editor.rebuildView = true;
                        }
                    }
                }
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                {
                    int worldTileX = tileX + windowX;
                    int worldTileY = tileY;

                    int index = worldTileY * fullMetaWidth + worldTileX;

                    if (index >= 0 && index < editor.levelData.size() && editor.inLevelRegion)
                    {
                        editor.selectedTile = editor.levelData[index];
                    }
                }
            }
        }
    }
    else
    {
        int screenA = scrollScreen;
        int screenB = scrollScreen + 1;

        if (hovering && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !editor.dragging)
        {
            editor.selectedObject = -1;
            editor.objectType = -1;
        }

        for (int type = 0; type < 2; ++type)
        {
            std::vector<Object>* objs = (type == 0 ? &editor.enemyData : &editor.itemData);
            int index = 0;

            for (Object& obj : *objs)
            {
                bool onScreen = (obj.screen == screenA || obj.screen == screenB);
                bool valid = (obj.type != 0xFF);

                if (!onScreen || !valid)
                {
                    ++index;
                    continue;
                }

                float px = min.x + (obj.screen - screenA) * 256 + obj.x;
                float py = min.y + obj.y;

                ImVec2 p0(px, py);
                ImVec2 p1(px + 8, py + 8);

                ImVec2 mouse = ImGui::GetMousePos();
                bool hovered = (mouse.x >= p0.x && mouse.x <= p1.x &&
                    mouse.y >= p0.y && mouse.y <= p1.y);

                if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                {
                    editor.selectedObject = index;
                    editor.objectType = type;
                    editor.dragging = true;
                    editor.dragOffset = ImVec2(mouse.x - px, mouse.y - py);
                    //std::println("{} {}", index, type);
                }

                if (editor.dragging &&
                    editor.objectType == type &&
                    editor.selectedObject == index &&
                    ImGui::IsMouseDown(ImGuiMouseButton_Left))
                {
                    float newX = mouse.x - editor.dragOffset.x - min.x - (obj.screen - screenA) * 256;
                    float newY = mouse.y - editor.dragOffset.y - min.y;

                    obj.x = std::clamp((int)newX, 0, 255);
                    obj.y = std::clamp((int)newY, 0, 255);
                }

                if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
                    editor.dragging = false;

                bool isSelected = (editor.objectType == type && editor.selectedObject == index);

                ImU32 col = isSelected
                    ? IM_COL32(255, 255, 0, 255)
                    : (type == 0 ? IM_COL32(255, 0, 0, 255) : IM_COL32(0, 0, 255, 255));

                dl->AddRect(p0, p1, col, 0.0f, 0, 2.0f);

                char buf[8];
                snprintf(buf, sizeof(buf), "%02d", obj.type);
                DrawTextOutlined(dl, ImVec2(px + 1, py + 1), IM_COL32(0, 0, 0, 255), buf);

                ++index;
            }
        }

        if (editor.selectedObject > -1)
        {
            std::vector<Object>* objs =
                (editor.objectType == 0 ? &editor.enemyData : &editor.itemData);

            Object& obj = (*objs)[editor.selectedObject];

            std::string text = "Selected Object: ";
            text += (editor.objectType == 0 ? "Enemy" : "Item");
            ImGui::SeparatorText(text.c_str());

            int x = obj.x, y = obj.y, scr = obj.screen, type = obj.type;

            ImGui::InputInt("X", &x);
            ImGui::InputInt("Y", &y);
            ImGui::InputInt("Screen #", &scr);
            ImGui::InputInt("Type", &type);

            obj.x = std::clamp(x, 0, 255);
            obj.y = std::clamp(y, 0, 255);
            obj.screen = std::clamp(scr, 0, numScreens - 1);
            obj.type = std::clamp(type, 0, 255);

            if (obj.screen > scrollScreen + 1 || obj.screen < scrollScreen)
            {
                scrollScreen = obj.screen;
                editor.rebuildView = true;
            }
        }
    }
}

bool App::drawScrollData(std::vector<uint8_t>& data, int screenNum)
{
    int index = 0;
    int screens = 0;
    uint8_t scrollByte = 0;
    int screenAmount = 0;

    while (index < data.size())
    {
        scrollByte = data[index];
        screenAmount = (scrollByte & 0x0F) + 1;
        screens += screenAmount;

        if (screens > screenNum)
            break;

        ++index;
    }

    uint8_t flags = scrollByte & 0xF0;

    constexpr FlagItem items[] = {
        { "Scroll Right ",      0x10 },
        { "Right Edge ",        0x20 },
        { "Bottom Edge ",       0x40 },
        { "Top Edge ",          0x80 }
    };

    ImGui::Text("Screen Transition Type at scroll stop:");

    std::string label;

    for (auto& item : items)
    {
        bool checked = flags & item.bit;

        label = item.name;
        label += std::to_string(screenNum).c_str();
        if (ImGui::Checkbox(label.c_str(), &checked))
        {
            if (checked)
                flags |= item.bit;
            else
                flags &= ~item.bit;

            scrollByte = (scrollByte & 0x0F) | flags;
        }
    }

     label = "Screen Amount " + std::to_string(screenNum);

    if (ImGui::BeginCombo(label.c_str(), std::to_string(screenAmount).c_str()))
    {
        for (int i = 1; i <= 16; ++i)
        {
            bool selected = (screenAmount == i);
            if (ImGui::Selectable(std::to_string(i).c_str(), selected))
            {
                screenAmount = i;
                scrollByte = (scrollByte & 0xF0) | ((screenAmount - 1) & 0x0F);
            }

            if (selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    data[index] = scrollByte;

    return screens - 1 == screenNum;
}

void App::saveROMData()
{
    auto& names = editor.data[editor.mode].levelNames;
    const std::string& levelName = names[editor.selectedLevel];
    const LevelEntry& level = editor.data[editor.mode].levels.at(levelName);

    if (!editor.levelData.empty())
        saveLevelData(editor.rom, level.map, editor.levelData);

    if (!editor.scrollData.empty())
        saveScrollData(editor.rom, level.scroll, editor.scrollData);

    if (!editor.enemyData.empty())
        saveObjectData(editor.rom, level.enemy_screen, level.enemy_x, level.enemy_y, level.enemy_type, editor.enemyData);

    if (!editor.itemData.empty())
        saveObjectData(editor.rom, level.item_screen, level.item_x, level.item_y, level.item_type, editor.itemData);

    if (!editor.levelMetaTiles.empty())
        saveMetaTilesToROM(editor.rom, level.chip32x32, level.chip32x32_palette, editor.levelMetaTiles);
}

void App::shutdown()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DestroyContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
