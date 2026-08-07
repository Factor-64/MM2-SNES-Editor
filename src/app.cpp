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

    while (!quit)
    {
        uint64_t now = SDL_GetPerformanceCounter();
        double delta = (double)(now - lastTime) / SDL_GetPerformanceFrequency();
        lastTime = now;

        animAccumulator += delta;

        while (animAccumulator >= (1.0 / 60.0))
        {
            updatePaletteAnimation();
            updateScollPreview();
            animAccumulator -= (1.0 / 60.0);
        }

        while (SDL_PollEvent(&e))
        {
            ImGui_ImplSDL3_ProcessEvent(&e);

            if (e.type == SDL_EVENT_QUIT)
                quit = true;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();

        ImGui::NewFrame();

        ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::DockSpaceOverViewport(vp->ID);

        if (ImGui::IsKeyPressed(ImGuiKey_B))
        {
            editor.paintMode = !editor.paintMode;
        }

        drawMenu(menuState, editor.romLoaded, editor.jsonLoaded && editor.paletteLoaded, (editor.mode == 0 || exportingAllData));

        switch (menuState)
        {
        case MS_OpenLoadJson: {
            if (!ImGuiFileDialog::Instance()->IsOpened())
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
            menuState = MS_NULL;
            break;
        }
        case MS_OpenLoadROM: {
            if (!ImGuiFileDialog::Instance()->IsOpened())
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
            menuState = MS_NULL;
            break;
        }
        case MS_OpenLoadPal: {
            if (!ImGuiFileDialog::Instance()->IsOpened())
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
            menuState = MS_NULL;
            break;
        }
        case MS_OpenExportROM: {
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
            menuState = MS_NULL;
            break;
        }
        case MS_OpenHeaderWindow: {
            openHeader = true;
            menuState = MS_NULL;
            break;
        }
        case MS_ImportData: {
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
            menuState = MS_NULL;
            break;
        }
        case MS_ExportGraphics: {
            saveROMData();
            saveBinary(LF_LevelGFX, editor.selectedLevel, editor.mode);
            break;
        }
        case MS_ExportLayer2TilemapData: {
            saveROMData();
            saveBinary(LF_Layer2TilemapData, editor.selectedLevel, editor.mode);
            menuState = MS_NULL;
            break;
        }
        case MS_ExportLayer3TilemapData: {
            saveROMData();
            saveBinary(LF_Layer3TilemapData, editor.selectedLevel, editor.mode);
            menuState = MS_NULL;
            break;
        }
        case MS_ExportCommonGFX: {
            saveROMData();
            saveBinary(LF_CommonGFX, editor.selectedLevel, editor.mode);
            break;
        }
        case MS_ExportMetaTiles: {
            saveROMData();
            saveBinary(LF_MetaTileData, editor.selectedLevel, editor.mode);
            menuState = MS_NULL;
            break;
        }
        case MS_ExportMetaTilePal: {
            saveROMData();
            saveBinary(LF_MetaTilePaletteData, editor.selectedLevel, editor.mode);
            menuState = MS_NULL;
            break;
        }
        case MS_ExportLayout: {
            saveROMData();
            saveBinary(LF_LevelData, editor.selectedLevel, editor.mode);
            menuState = MS_NULL;
            break;
        }
        case MS_ExportCollision: {
            saveROMData();
            saveBinary(LF_CollisionData, editor.selectedLevel, editor.mode);
            menuState = MS_NULL;
            break;
        }
        case MS_ExportScroll: {
            saveROMData();
            saveBinary(LF_ScrollData, editor.selectedLevel, editor.mode);
            menuState = MS_NULL;
            break;
        }
        case MS_ExportEnemy: {
            saveROMData();
            saveBinary(LF_EnemyData, editor.selectedLevel, editor.mode);
            menuState = MS_NULL;
            break;
        }
        case MS_ExportItem: {
            saveROMData();
            saveBinary(LF_ItemData, editor.selectedLevel, editor.mode);
            menuState = MS_NULL;
            break;
        }
        case MS_ExportMidpoint: {
            saveROMData();
            saveBinary(LF_MidPointData, editor.selectedLevel, editor.mode);
            menuState = MS_NULL;
            break;
        }
        case MS_ExportPatternTable: {
            saveROMData();
            saveBinary(LF_PatternTable, editor.selectedLevel, editor.mode);
            menuState = MS_NULL;
            break;
        }
        case MS_ExportPalette: {
            saveROMData();
            saveBinary(LF_PaletteData, editor.selectedLevel, editor.mode);
            menuState = MS_NULL;
            break;
        }
        case MS_ExportLayer2Palette: {
            saveROMData();
            saveBinary(LF_Layer2PaletteData, editor.selectedLevel, editor.mode);
            menuState = MS_NULL;
            break;
        }
        case MS_ExportLayer3Palette: {
            saveROMData();
            saveBinary(LF_Layer2PaletteData, editor.selectedLevel, editor.mode);
            menuState = MS_NULL;
            break;
        }
        case MS_ExportPaletteAnimation: {
            saveROMData();
            saveBinary(LF_PaletteAnimationData, editor.selectedLevel, editor.mode);
            break;
        }
        case MS_ExportAllData: {
            saveROMData();
            exportingAllData = true;
            break;
        }
        case MS_ExportBGScrollSpeedData:
            saveROMData();
            saveBinary(LF_BGScrollSpeedData, editor.selectedLevel, editor.mode);
            menuState = MS_NULL;
            break;
        case MS_ExportBGTilemapMirroring:
            saveROMData();
            saveBinary(LF_BGTilemapMirroring, editor.selectedLevel, editor.mode);
            menuState = MS_NULL;
            break;
        case MS_ExportBGScrollEnable:
            saveROMData();
            saveBinary(LF_BGScrollEnable, editor.selectedLevel, editor.mode);
            menuState = MS_NULL;
            break;
        default:
            menuState = MS_NULL;
            break;
        }

        if (ImGuiFileDialog::Instance()->IsOpened())
        {
            ImVec2 vp = ImGui::GetMainViewport()->Size;
            ImVec2 minSize = ImVec2(vp.x * 0.6f, vp.y * 0.6f);
            std::string key = ImGuiFileDialog::Instance()->GetOpenedKey();

            if (ImGuiFileDialog::Instance()->Display(
                key,
                ImGuiWindowFlags_NoCollapse,
                minSize,
                vp
            ))
            {
                if (ImGuiFileDialog::Instance()->IsOk())
                {
                    std::string path = ImGuiFileDialog::Instance()->GetFilePathName();

                    if (key == "ChooseROM")
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
                    else if (key == "ChooseJSON")
                    {
                        editor.data = loadMM2Data(path, editor.isHiROM);
                        editor.jsonLoaded = true;
                        editor.rebuildData = true;
                    }
                    else if (key == "ChoosePalette")
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
                            ++currentExportIndex;
                        else if (currentExportIndex < -1)
                            menuState = MS_ExportPaletteAnimation;
                        else if (menuState == MS_NULL)
                            exportingData = false;
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
                    menuState = MS_NULL;
                    exportingData = false;
                    exportingAllData = false;
                    currentExportIndex = -1;
                }

                ImGuiFileDialog::Instance()->Close();
            }
        }

        if (editor.jsonLoaded && editor.romLoaded && editor.paletteLoaded)
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
            if (openHeader)
                drawHeaderWindow();
        }

        /*if (menuState.openSettings)
        {
            ImGui::Begin("Settings", &open);
            ImGui::Text("Preferences go here...");
            if (ImGui::Button("Close"))
                menuState.openSettings = false;
            ImGui::End();
        }*/

        if (exportingAllData)
        {
            static LevelField currentField = LF_CommonGFX;
            if (!exportingData)
                currentField = static_cast<LevelField>(static_cast<int>(currentField) + 1);
            saveBinary(currentField, editor.selectedLevel, editor.mode);
            if (currentField >= LF_END)
            {
                menuState = MS_NULL;
                exportingAllData = false;
                currentField = LF_CommonGFX;
            }
        }

        ImGui::Render();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
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

void App::updateScollPreview()
{
    BGSpeedData* l2;
    BGSpeedData* l3;
    if (!editor.scrollLayer2Vertical)
        l2 = &editor.bgScrollSpeeds[editor.currentScreenId][0];
    else
        l2 = &editor.bgScrollSpeeds[editor.currentScreenId][2];
    if (!editor.scrollLayer3Vertical)
        l3 = &editor.bgScrollSpeeds[editor.currentScreenId][1];
    else
        l3 = &editor.bgScrollSpeeds[editor.currentScreenId][3];
    if (l2->frames != 0)
    {
        --l2->frame_count;
        if (l2->frame_count == 0)
        {
            l2->frame_count = l2->frames;
            editor.layer2Scanlines += l2->scanlines;
        }
    }
    else
    {
        editor.layer2Scanlines += l2->scanlines;
    }
    if (l3->frames != 0)
    {
        --l3->frame_count;
        if (l3->frame_count == 0)
        {
            l3->frame_count = l3->frames;
            editor.layer3Scanlines += l3->scanlines;
        }
    }
    else
    {
        editor.layer3Scanlines += l2->scanlines;
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
            currentExportIndex = 0;

        if (currentExportIndex >= levelGfx.layer12.size())
        {
            currentExportIndex = -1;
            menuState = MS_NULL;
            exportingData = false;
            return;
        }

        uint32_t start = levelGfx.layer12[currentExportIndex].start;
        uint32_t end = levelGfx.layer12[currentExportIndex].end;

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
    case LF_Layer2TilemapData:
        offset = level.bg_tilemap;
        size = 0x1000;
        break;
    case LF_Layer3TilemapData:
        offset = level.bg_tilemap + 0x1000;
        size = 0x1000;
        break;
    case LF_BGScrollEnable:
        offset = level.bg_scroll;
        size = 0x40;
        break;
    case LF_BGScrollSpeedData:
        offset = level.bg_speed;
        size = 0x80;
        break;
    case LF_BGTilemapMirroring:
        offset = level.bg_mirror;
        size = 0x80;
        break;
    case LF_LevelData:
        offset = level.map;
        size = 0xB00;
        break;
    case LF_CollisionData:
        offset = level.collision;
        size = 0x400;
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
    case LF_Layer2PaletteData:
        offset = level.palette_layer2;
        size = 32;
        break;
    case LF_Layer3PaletteData:
        offset = level.palette_layer3;
        size = 32;
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
                menuState = MS_NULL;
                exportingData = false;
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
                menuState = MS_NULL;
                return;
            }

            int frame = currentExportIndex;

            uint32_t ptr = level.palette_anime + 1 + frame * 2;
            uint16_t src = editor.rom[ptr] | (editor.rom[ptr + 1] << 8);

            if (src == 0xFFFF)
            {
                ++currentExportIndex;
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

    if (offset + size > editor.rom.size() || offset == 0)
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
                if (editor.mode == 0 && (editor.tileViewMode == VM_Layer2 || editor.tileViewMode == VM_Layer3))
                    editor.tileViewMode = VM_Tileset;
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
            levelGfx.layer12.insert(
                levelGfx.layer12.begin(),
                commonGfx.layer12.begin(),
                commonGfx.layer12.end()
            );
            /*for (const auto& g : commonGfx.layer12)
            {
                ImGui::Text("Common GFX Range: %06X - %06X", g.start, g.end);
                ++common;
            }*/
        }
        else
        {
            for (auto it = levelGfx.commonIdx.rbegin(); it != levelGfx.commonIdx.rend(); ++it)
            {
                size_t i = *it;
                if (i >= commonGfx.layer12.size())
                    throw std::runtime_error("commonIdx index out of range");

                levelGfx.layer12.insert(levelGfx.layer12.begin(), commonGfx.layer12[i]);

                //ImGui::Text("Common GFX Range: %06X - %06X", commonGfx.ranges[i].start, commonGfx.ranges[i].end);
                //++common;
            }
        }
    }

    if (editor.rebuildData)
    {
        editor.rebuildData = false;

        editor.rebuildView = true;
        editor.rebuildTileset = true;
        editor.rebuildEdit = true;
        editor.rebuildBackgrounds = true;

        editor.levelTiles = decodeTileRanges(levelGfx.layer12, editor.rom, 32);
        
        editor.levelTileMap = makeTileMap(editor.levelTiles, 16, 1);

        std::vector<MetaTileData> levelMetaTileData;

        if (editor.mode)
        {
            editor.layer3Tiles = decodeTileRanges(levelGfx.layer3, editor.rom, 16);
            editor.layer2TileMap = makeTileMap(editor.levelTiles, 16, 0);
            editor.layer3TileMap = makeTileMap(editor.layer3Tiles, 16, 0);

            editor.layer2TileData = loadBackgroundTileData(editor.rom, level.bg_tilemap, 0x1000);
            editor.layer3TileData = loadBackgroundTileData(editor.rom, level.bg_tilemap + 0x1000, 0x1000);
            levelMetaTileData = decodeMetaTile32SNES(editor.rom, level.chip32x32, level.collision, 0x400);
            editor.palettes = decodeCGRAMPalettes(editor.rom, level.palette_data, 16);
            Palettes p = decodeCGRAMPalettes(editor.rom, level.palette_layer3, 2);
            editor.palettes[0] = p[0];
            editor.palettes[1] = p[1];
            p = decodeCGRAMPalettes(editor.rom, level.palette_layer2, 2);
            editor.palettes[6] = p[0];
            editor.palettes[7] = p[1];
            editor.animate = loadAnimatedPalettes(editor.rom, level.palette_afc, level.palette_aft, level.palette_anime, editor.isHiROM, editor.palettes);
            if ((editor.animate.palette_bits & 0x30) != 0x30)
            {
                if (editor.animate.frame_count)
                {
                    editor.animate.palette_bits |= 0x30;
                    for (int i = 0; i < editor.animate.frame_count; ++i)
                    {
                        int base = i * 6;
                        editor.animate.frames[base + 4] = p[0];
                        editor.animate.frames[base + 5] = p[1];
                        editor.animate.palette_bits |= 0x30;
                    }
                }
            }

            editor.bgScrollData = loadBackgroundScrollData(editor.rom, level.bg_scroll);
            editor.bgPositionData[0] = loadBGPositionData(editor.rom, level.bg_start);
            editor.bgPositionData[1] = loadBGPositionData(editor.rom, level.bg_checkpoint);
            editor.bgPositionData[2] = loadBGPositionData(editor.rom, level.bg_boss);

            editor.bgTilemapMirror = loadBGTilemapMirror(editor.rom, level.bg_mirror);

            editor.bgScrollSpeeds = loadBGScrollSpeeds(editor.rom, level.bg_speed);
        }
        else
        {
            levelMetaTileData = decodeMetaTile32NES(editor.rom, level.chip32x32, 0x400);
            editor.palettes = makeNESPalettes(editor.rom, level.palette_data, editor.nesMasterPalette);
            editor.animate = loadAnimatedPalettesNES(editor.rom, level.palette_afc, level.palette_aft, level.palette_anime, editor.nesMasterPalette);
        }

        loadMetaTilePalettes(levelMetaTileData, editor.rom, level.chip32x32_palette, 0x100);

        editor.aniPalettes = editor.palettes;
        if(editor.mode == 0)
            editor.paletteIndex = 0;
        else
            editor.paletteIndex = 2;
        editor.aniPalIndex = 0;

        editor.subPalettes = editor.palettes;
        editor.subPaletteIndex = 0;

        editor.levelMacroTiles = buildMacroTiles(editor.levelTileMap);
        editor.levelMetaTiles = makeMetaTiles(levelMetaTileData, editor.levelMacroTiles);

        editor.levelData = loadLevelData(editor.rom, level.map, 0xB00);
        editor.scrollData = loadScrollData(editor.rom, level.scroll, 0x200);
        editor.levelData = remapColumnMajorScreensHorizontally(editor.levelData);

        editor.enemyData = loadObjectData(editor.rom, level.enemy_screen, level.enemy_x, level.enemy_y, level.enemy_type, 0x100);
        editor.itemData = loadObjectData(editor.rom, level.item_screen, level.item_x, level.item_y, level.item_type, 0x40);
    }

    /*for (size_t i = common; i < levelGfx.ranges.size(); ++i)
        ImGui::Text("GFX Range: %06X - %06X", levelGfx.ranges[i].start, levelGfx.ranges[i].end);

    ImGui::Text("chip32x32: %06X", level.chip32x32);
    ImGui::Text("palette:   %06X", level.chip32x32_palette);
    ImGui::Text("pattern:   %06X", level.pattern);*/

    static bool force = false;
    const char* tabNames[] = { "Tileset", "Meta Tiles", "Collision", "Background Layer 2", "Background Layer 3"};
    if (ImGui::BeginCombo("Tile Mode", tabNames[editor.tileViewMode]))
    {
        int tabsize = editor.mode == 0 ? 3 : 5;
        for (int i = 0; i < tabsize; ++i)
        {
            bool selected = (i == editor.tileViewMode);
            if (ImGui::Selectable(tabNames[i], selected))
            {
                editor.tileViewMode = static_cast<ViewMode>(i);
                editor.selectedTile = -1;
                switch (editor.tileViewMode)
                {
                    case VM_Tileset:
                        if (editor.paletteIndex < 2 || editor.paletteIndex > 5)
                            editor.paletteIndex = 2;
                        editor.editMode = EM_Metatiles;
                        break;
                    case VM_Metatiles:
                        editor.editMode = EM_Level;
                        break;
                    case VM_Collision:
                        editor.editMode = EM_Collision;
                        break;
                    case VM_Layer2:
                        editor.editMode = EM_Layer2;
                        break;
                    case VM_Layer3:
                        if (editor.paletteIndex > 1)
                            editor.paletteIndex = 0;
                        editor.editMode = EM_Layer3;
                        break;
                    default:
                        break;
                }
                editor.rebuildBackgrounds = true;
                editor.rebuildEdit = true;
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
        drawTileView();
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
        editor.rebuildBackgrounds = true;
        editor.rebuildEdit = true;
        editor.rebuildView = true;
    }
}

void App::DrawColorButton(const std::string& id, ColorRGBA& col, const PaletteType type, size_t paletteIndex, int colorIndex, const char* popupName, const LevelEntry& level, ImVec2 size)
{
    ImVec4 c(col.r / 255.f, col.g / 255.f, col.b / 255.f, 1.f);

    if (ImGui::ColorButton(id.c_str(), c, 0, size))
    {
        editor.editingPalette = paletteIndex;
        editor.editingColor = colorIndex;
        editor.tempColor = col;
        editor.paletteType = type;
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
            editor.paletteType = type;
            editor.tempColor = editor.colClip.color;
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
            editor.paletteType = type;
            if (editor.mode == 0)
                writeNESColorToROM(level, -1);
            else
                writeSNESColorToROM(level);
        }

        ImGui::EndPopup();
    }

    ImGui::SameLine();
}

void App::DrawPaletteRow(const char* label, size_t index, Palette& pal, int colorsPerPalette, const PaletteType type, const char* popupName, const char* ident, const LevelEntry& level)
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
            editor.paletteType = type;
            if (editor.mode == 0)
                writeNESPaletteToROM(index, pal, level);
            else
                writeSNESPaletteToROM(index, pal, level);
        }

        if (ImGui::MenuItem("Swap With Clipboard", nullptr, false, editor.palClip.hasData))
        {
            std::swap(pal, editor.palClip.colors);
            editor.paletteType = type;
            if (editor.mode == 0)
                writeNESPaletteToROM(index, pal, level);
            else
                writeSNESPaletteToROM(index, pal, level);
        }

        ImGui::EndPopup();
    }

    for (int i = 0; i < colorsPerPalette; ++i)
    {
        id = std::string(ident) + "_" + std::to_string(index) + "_" + std::to_string(i);

        DrawColorButton(id, pal[i], type, index, i, popupName, level, colorsPerPalette == 16 ? ImVec2(20, 20) : ImVec2(0, 0));
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
        PaletteType type = PT_Normal;

        if (p < 2)
        {
            label = "L3 Palette";
            id = "L3";
            type = PT_Layer3;
        }
        else if (p < 6)
        {
            label = "L1 Palette";
            id = "L1";
        }
        else if (p < 8)
        {
            label = "L2 Palette";
            id = "L2";
            type = PT_Layer2;
        }
        else
        {
            label = "SPR Palette";
            id = "SPR";
        }

        size_t index = (p < half) ? p : p - half;

        DrawPaletteRow(label, index, editor.palettes[p], colorsPerPalette, type, popupName, id, level);
    }
}

void App::DrawAnimatedPalettes(int colorsPerPalette, int colorsPerFrame, const char* popupName, const LevelEntry& level)
{
    float newWidth = ImGui::GetContentRegionAvail().x * 0.20f;
    ImGui::PushItemWidth(newWidth);
    int frameCount = editor.animate.frame_count;
    ImGui::SeparatorText("Palette Animation");

    if (ImGui::InputInt("Frame Count", &frameCount))
    {
        frameCount = std::clamp(frameCount, 0, 4);
        editor.animate.frame_count = frameCount;
        editor.rom[level.palette_afc] = frameCount;
        editor.animate = loadAnimatedPalettes(editor.rom, level.palette_afc, level.palette_aft, level.palette_anime, editor.isHiROM, editor.palettes);
        if (editor.mode != 0)
        {
            if ((editor.animate.palette_bits & 0x30) != 0x30)
            {
                if (editor.animate.frame_count)
                {
                    editor.animate.palette_bits |= 0x30;
                    Palettes p = decodeCGRAMPalettes(editor.rom, level.palette_layer2, 2);
                    for (int i = 0; i < editor.animate.frame_count; ++i)
                    {
                        int base = i * 6;
                        editor.animate.frames[base + 4] = p[0];
                        editor.animate.frames[base + 5] = p[1];
                    }
                }
            }
        }
    }

    ImGui::SameLine();

    if (frameCount == 0)
        return;

    int frameTimer = editor.animate.frame_timer;
    if (ImGui::InputInt("Frame Timer", &frameTimer))
    {
        editor.animate.frame_timer = std::clamp(frameTimer, 0, 255);
        editor.rom[level.palette_aft] = editor.animate.frame_timer;
    }

    ImGui::PopItemWidth();

    bool changed = false;
    if (ImGui::BeginCombo("Frame", std::to_string(editor.aniPalIndex).c_str()))
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

    bool checked = ImGui::Checkbox("Animate Palettes", &editor.animatePalettes);

    if ((checked || changed) && !editor.animatePalettes)
    {
        editor.animFrame = editor.aniPalIndex;
        applyAnimationFrame();
        editor.rebuildTileset = true;
        editor.rebuildBackgrounds = true;
        editor.rebuildEdit = true;
        editor.rebuildView = true;
    }

    size_t start = editor.aniPalIndex * colorsPerFrame;
    size_t end = start + colorsPerFrame;

    for (size_t p = start; p < end && p < editor.animate.frames.size(); ++p)
        DrawPaletteRow("ANI Palette", p, editor.animate.frames[p], colorsPerPalette, PT_Animated, popupName, "ANI", level);
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
    switch (editor.paletteType)
    {
        case PT_Animated:
            base = level.palette_anime;
            break;
    }

    uint32_t addr = base + editor.editingPalette * 4 + editor.editingColor;

    editor.rom[addr] = index;

    switch (editor.paletteType)
    {
        case PT_Animated:
            editor.palettes = makeNESPalettes(editor.rom, base, editor.nesMasterPalette);
            editor.aniPalettes = editor.palettes;
            break;
        case PT_Normal:
            editor.animate = loadAnimatedPalettesNES(editor.rom, level.palette_afc, level.palette_aft, level.palette_anime, editor.nesMasterPalette);
            break;
    }

    applyAnimationFrame();

    editor.rebuildTileset = true;
    editor.rebuildBackgrounds = true;
    editor.rebuildEdit = true;
    editor.rebuildView = true;
}

void App::writeSNESColorToROM(const LevelEntry& level)
{
    switch (editor.paletteType)
    {
        case PT_Animated:
            editor.animate.frames[editor.editingPalette][editor.editingColor] = editor.tempColor;
            writeAnimatedPalettes(editor.rom, level.palette_anime, editor.isHiROM, editor.animate);
            editor.animate = loadAnimatedPalettes(editor.rom, level.palette_afc, level.palette_aft, level.palette_anime, editor.isHiROM, editor.palettes);
            break;
        case PT_Layer2: {
            uint32_t base = level.palette_layer2;
            uint32_t offset = editor.editingPalette == 7 ? 1 : 0;
            uint32_t addr = base + (offset * 16 + editor.editingColor) * 2;

            writeSNESColor(editor.rom, addr, editor.tempColor);

            Palettes p = decodeCGRAMPalettes(editor.rom, base, 2);
            editor.palettes[6] = p[0];
            editor.palettes[7] = p[1];
            break;
        }
        case PT_Layer3: {
            uint32_t base = level.palette_layer3;
            uint32_t addr = base + (editor.editingPalette * 16 + editor.editingColor) * 2;

            writeSNESColor(editor.rom, addr, editor.tempColor);

            Palettes p = decodeCGRAMPalettes(editor.rom, base, 2);
            editor.palettes[0] = p[0];
            editor.palettes[1] = p[1];
            for (int p = 0; p < 2; ++p)
            {
                for (int chunk = 0; chunk < 8; ++chunk)
                {
                    int dstIndex = p * 8 + chunk;
                    int offset = chunk * 4;

                    for (int c = 0; c < 4; ++c)
                    {
                        editor.subPalettes[dstIndex][c] = editor.palettes[p][offset + c];
                    }
                }
            }
            break;
        }
        case PT_Normal: {
            uint32_t base = level.palette_data;
            uint32_t addr = base + (editor.editingPalette * 16 + editor.editingColor) * 2;

            writeSNESColor(editor.rom, addr, editor.tempColor);

            editor.palettes = decodeCGRAMPalettes(editor.rom, base, 16);
            Palettes p = decodeCGRAMPalettes(editor.rom, level.palette_layer3, 2);
            editor.palettes[0] = p[0];
            editor.palettes[1] = p[1];
            p = decodeCGRAMPalettes(editor.rom, level.palette_layer2, 2);
            editor.palettes[6] = p[0];
            editor.palettes[7] = p[1];
            break;
        }
    }

    editor.aniPalettes = editor.palettes;

    applyAnimationFrame();

    editor.rebuildTileset = true;
    editor.rebuildBackgrounds = true;
    editor.rebuildEdit = true;
    editor.rebuildView = true;
}

void App::writeNESPaletteToROM(size_t paletteIndex, const Palette& pal, const LevelEntry& level)
{
    uint32_t base = level.palette_data;
    switch (editor.paletteType)
    {
        case PT_Animated:
            base = level.palette_anime;
            break;
    }

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
    editor.rebuildBackgrounds = true;
    editor.rebuildEdit = true;
    editor.rebuildView = true;
}

void App::writeSNESPaletteToROM(size_t paletteIndex, const Palette& pal, const LevelEntry& level)
{
    switch(editor.paletteType)
    {
        case PT_Animated:
            editor.animate.frames[paletteIndex] = pal;
            writeAnimatedPalettes(editor.rom, level.palette_anime, editor.isHiROM, editor.animate);
            editor.animate = loadAnimatedPalettes(editor.rom, level.palette_afc, level.palette_aft, level.palette_anime, editor.isHiROM, editor.palettes);
            break;
        case PT_Layer2: {
            uint32_t base = level.palette_layer2;
            uint32_t offset = paletteIndex == 7 ? 1 : 0;

            for (int i = 0; i < 16; ++i)
            {
                uint32_t addr = base + (offset * 16 + i) * 2;
                writeSNESColor(editor.rom, addr, pal[i]);
            }

            Palettes p = decodeCGRAMPalettes(editor.rom, base, 2);
            editor.palettes[6] = p[0];
            editor.palettes[7] = p[1];
            break;
        }
        case PT_Layer3: {
            uint32_t base = level.palette_layer2;

            for (int i = 0; i < 16; ++i)
            {
                uint32_t addr = base + (paletteIndex * 16 + i) * 2;
                writeSNESColor(editor.rom, addr, pal[i]);
            }

            Palettes p = decodeCGRAMPalettes(editor.rom, base, 2);
            editor.palettes[6] = p[0];
            editor.palettes[7] = p[1];
            for (int p = 0; p < 2; ++p)
            {
                for (int chunk = 0; chunk < 8; ++chunk)
                {
                    int dstIndex = p * 8 + chunk;
                    int offset = chunk * 4;

                    for (int c = 0; c < 4; ++c)
                    {
                        editor.subPalettes[dstIndex][c] = editor.palettes[p][offset + c];
                    }
                }
            }
            break;
        }
        case PT_Normal: {
            uint32_t base = level.palette_data;
            for (int i = 0; i < 16; ++i)
            {
                uint32_t addr = base + (paletteIndex * 16 + i) * 2;
                writeSNESColor(editor.rom, addr, pal[i]);
            }
            editor.palettes = decodeCGRAMPalettes(editor.rom, base, 16);
            Palettes p = decodeCGRAMPalettes(editor.rom, level.palette_layer3, 2);
            editor.palettes[0] = p[0];
            editor.palettes[1] = p[1];
            p = decodeCGRAMPalettes(editor.rom, level.palette_layer2, 2);
            editor.palettes[6] = p[0];
            editor.palettes[7] = p[1];
            break;
        }
    }

    editor.aniPalettes = editor.palettes;

    applyAnimationFrame();

    editor.rebuildTileset = true;
    editor.rebuildBackgrounds = true;
    editor.rebuildEdit = true;
    editor.rebuildView = true;
}

void App::drawPaletteWindow()
{
    ImGui::Begin("Palettes", &open, ImGuiWindowFlags_HorizontalScrollbar);
    
    ImGui::Text("CONTROLS:");
    ImGui::Text("Left Click a color to change it");
    ImGui::Text("Left Click a palette name to copy, paste, or swap with clipboard");

    auto& names = editor.data[editor.mode].levelNames;
    const std::string& levelName = names[editor.selectedLevel];
    const LevelEntry& level = editor.data[editor.mode].levels.at(levelName);

    std::string label = "Palettes for ";
    label += levelName;

    ImGui::SeparatorText(label.c_str());

    bool isNES = (editor.mode == 0);

    int offset = isNES ? 0 : 2;
    int colorsPerFrame = isNES ? 4 : 6;
    int colorCount = 4 + offset;
    switch (editor.tileViewMode)
    {
        case VM_Layer2:
            colorCount = 8;
            offset = 0;
            break;
        case VM_Layer3:
            offset = 0;
            colorCount = 2;
            break;
        default:
            break;
    }

    if (ImGui::BeginCombo("BG Palette Index", std::to_string(editor.paletteIndex).c_str()))
    {
        for (int i = offset; i < colorCount; ++i)
        {
            bool selected = (i == editor.paletteIndex);

            if (ImGui::Selectable(std::to_string(i).c_str(), selected))
            {
                editor.paletteIndex = i;
                editor.rebuildTileset = true;
            }
            if (selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    if (editor.editMode == EM_Layer3)
    {
        if (ImGui::BeginCombo("Sub Palette Index", std::to_string(editor.subPaletteIndex).c_str()))
        {
            for (int i = 0; i < 4; ++i)
            {
                bool selected = (i == editor.subPaletteIndex);

                if (ImGui::Selectable(std::to_string(i).c_str(), selected))
                {
                    editor.subPaletteIndex = i;
                    editor.rebuildTileset = true;
                }
                if (selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
    }

    if (ImGui::Checkbox("Universal BG Color", &editor.universalBGColor))
    {
        editor.rebuildTileset = true;
        editor.rebuildBackgrounds = true;
        editor.rebuildEdit = true;
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

    if (editor.editMode != EM_Level)
    {
        ImGui::Text("CONTROLS:");
        ImGui::Text("CTRL+= and CTRL+- Zooms in and out");
        ImGui::Text("B to toggle Paint Mode");
        if (editor.editMode == EM_Collision)
        {
            ImGui::Text("Hold Left Click to Paint with the Selected Collision Type");
        }
        else
        {
            ImGui::Text("Hold Left Click to Paint with the Selected Tile");
            ImGui::Text("The selected Tile will use the currently Select Palette & Attributes");
            ImGui::Text("Right Click to grab the currently hovered Tile");
            ImGui::Text("Paint Mode makes Left Click only Paint with the currently Selected Palette & Attribute");
            ImGui::Checkbox("Paint Mode", &editor.paintMode);
            if (editor.editMode == EM_Layer2 || editor.editMode == EM_Layer3)
            {
                ImGui::Checkbox("Horizontal Flip", &editor.hFlip);
                ImGui::Checkbox("Vertical Flip", &editor.vFlip);
                ImGui::Checkbox("High Priority", &editor.hPriority);
            }
        }
        ImGui::Separator();
        drawEditMode();
    }
    else
    {
        drawLevelView();
    }

    ImGui::End();
}

void App::drawHeaderWindow()
{
    ImGui::Begin("SNES Header Info", &openHeader);

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

inline void DrawNearestImage(ImDrawList* dl, const TilemapTexture& tileset, const ImVec2& size, const ImVec2& c1, const ImVec2& c2, const ImVec2 pos = ImVec2(-1,-1))
{
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();

    dl->AddCallback(platform_io.DrawCallback_SetSamplerNearest, nullptr);

    if (pos.x > -1)
    {
        dl->AddImage((ImTextureID)(intptr_t)tileset.tex,
            pos,
            ImVec2(pos.x + size.x, pos.y + size.y),
            c1, c2);
    }
    else
    {
        ImGui::Image((ImTextureID)(intptr_t)tileset.tex, size, c1, c2);
    }

    dl->AddCallback(ImDrawCallback_ResetRenderState, nullptr);
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

    int offset = editor.mode == 0 ? 0 : 2;
    editor.levelMetaTiles[metaTileIndex].palettes[localIndex] = editor.paletteIndex - offset;

    editor.rebuildTileset = true;
    editor.rebuildEdit = true;
}

inline void App::PaintTileBackground(std::vector<BGTileData>& data, int tileX, int tileY, int atlasWidth, const bool color, const bool subPal)
{
    if (tileX < 0 || tileY < 0)
        return;

    int tileIndex = tileY * atlasWidth + tileX;

    if (tileIndex < 0 || tileIndex >= data.size())
        return;

    if (!color)
    {
        data[tileIndex].vramPage = editor.selectedTile >> 8;
        data[tileIndex].tileId = editor.selectedTile & 0xFF;
    }

    if (!subPal)
        data[tileIndex].palette = editor.paletteIndex;
    else
        data[tileIndex].palette = editor.subPaletteIndex;

    data[tileIndex].hFlip = editor.hFlip;
    data[tileIndex].vFlip = editor.vFlip;
    data[tileIndex].highPriority = editor.hPriority;

    editor.rebuildEdit = true;
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

inline void App::PaintMacroTilePixel(int macroIndex, int x, int y)
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
    editor.rebuildEdit = true;
}

inline void App::PaintTilePixel(int tileIndex, int x, int y)
{
    Tile& t = editor.levelTiles[tileIndex];

    y = y % 8;
    x = x % 8;

    t.pixels[y * 8 + x] = editor.selectedColor;

    saveTileToROM(t, editor.rom);

    editor.rebuildTileset = true;
    editor.rebuildEdit = true;
}

void verticalMirroring(std::vector<ColorRGBA>& pixels, int& width, int& height, bool repeat)
{
    int halfH = height / 2;
    int newW = width * 2;

    std::vector<ColorRGBA> stitched(newW * halfH);
    for (int y = 0; y < halfH; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            stitched[y * newW + x] = pixels[y * width + x];
        }
    }
    if (!repeat)
    {
        for (int y = 0; y < halfH; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                stitched[y * newW + (x + width)] = pixels[(y + halfH) * width + x];
            }
        }
    }
    else
    {
        for (int y = 0; y < halfH; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                stitched[y * newW + (x + width)] = pixels[y * width + x];
            }
        }
    }

    pixels = stitched;
    width = newW;
    height = halfH;
}

void horizontalMirroring(std::vector<ColorRGBA>& pixels, int& width, int& height)
{
    int halfH = height / 2;

    for (int y = 0; y < halfH; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            pixels[(y + halfH) * width + x] = pixels[y * width + x];
        }
    }
}

void DrawHorizontalWrappingImage(ImDrawList* dl, TilemapTexture& t, int& trueWidth, int& trueHeight, uint32_t& scanlines, int& zoom)
{
    int hW = trueWidth / 2;

    float uvOffset = fmod((float)(scanlines * zoom) / t.width, 1.0f);
    if (uvOffset < 0) uvOffset += 1.0f;

    float u0 = uvOffset;
    float u1 = uvOffset + 0.5f;

    if (u1 <= 1.0f)
    {
        DrawNearestImage(dl, t,
            ImVec2(hW, trueHeight),
            ImVec2(u0, 0),
            ImVec2(u1, 1)
        );
    }
    else
    {
        float u1_wrapped = u1 - 1.0f;
        float w0 = 1.0f - u0;        // width of first UV slice
        float w1 = u1_wrapped;       // width of second UV slice
        float total = w0 + w1;       // should be 0.5
        float px0 = hW * (w0 / total);
        //float px1 = hW * (w1 / total);
        int px0i = (int)(px0 + 0.5f);
        int px1i = hW - px0i;

        DrawNearestImage(dl, t,
            ImVec2(px0i, trueHeight),
            ImVec2(u0, 0),
            ImVec2(1.0f, 1)
        );

        ImGui::SameLine(0, 0);

        DrawNearestImage(dl, t,
            ImVec2(px1i, trueHeight),
            ImVec2(0.0f, 0),
            ImVec2(u1_wrapped, 1)
        );
    }
}

void DrawVerticalWrappingImage(ImDrawList* dl, TilemapTexture& t, int& trueWidth, int& trueHeight, uint32_t& scanlines, int& zoom)
{
    
    int hH = trueHeight / 2;

    float uvOffset = fmod((float)(scanlines * zoom) / t.height, 1.0f);
    if (uvOffset < 0) uvOffset += 1.0f;

    float v0 = uvOffset;
    float v1 = uvOffset + 0.5f;

    if (v1 <= 1.0f)
    {
        DrawNearestImage(dl, t,
            ImVec2(trueWidth, hH),
            ImVec2(0, v0),
            ImVec2(1, v1)
        );
    }
    else
    {
        float v1_wrapped = v1 - 1.0f;
        float h0 = 1.0f - v0;        // height of first UV slice
        float h1 = v1_wrapped;       // height of second UV slice
        float total = h0 + h1;       // should be 0.5
        float px0 = hH * (h0 / total);
        //float px1 = hH - px0;
        int px0i = (int)(px0 + 0.5f); 
        int px1i = hH - px0i;

        DrawNearestImage(dl, t,
            ImVec2(trueWidth, px0i),
            ImVec2(0, v0),
            ImVec2(1, 1.0f)
        );

        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImGuiStyle& style = ImGui::GetStyle();
        ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y - style.ItemSpacing.y));

        DrawNearestImage(dl, t,
            ImVec2(trueWidth, px1i),
            ImVec2(0, 0.0f),
            ImVec2(1, v1_wrapped)
        );
    }
}

void App::drawEditMode()
{
    int tileSize = 8;
    float scale = 2.0f;
    static TilemapTexture tilegrid;

    switch (editor.editMode)
    {
        case EM_Collision:
        case EM_Metatiles: {
            tileSize = 16;
            scale = 1.0f;
            break;
        }
        default:
            break;
    }

    if (editor.rebuildEdit || tilegrid.tex == 0)
    {
        editor.rebuildEdit = false;

        if (tilegrid.tex != 0)
            glDeleteTextures(1, &tilegrid.tex);

        std::vector<ColorRGBA> outPixels;

        ColorRGBA bgColor = editor.palettes[0][0];
        if (!editor.universalBGColor)
            bgColor.a = 0;

        switch (editor.editMode)
        {
            case EM_Layer2:
                renderBGTileMapToRGBA(editor.layer2TileData, 32, editor.levelTiles, editor.aniPalettes, bgColor, outPixels, tilegrid.width, tilegrid.height);
                break;
            case EM_Layer3:
                renderBGTileMapToRGBA(editor.layer3TileData, 32, editor.layer3Tiles, editor.subPalettes, bgColor, outPixels, tilegrid.width, tilegrid.height);
                break;
            case EM_Collision:
            case EM_Metatiles: {
                uint8_t offset = editor.mode == 0 ? 0 : 2;
                renderMetaTileMapToRGBA(editor.levelMetaTiles, 16, editor.levelTiles, editor.aniPalettes, offset, bgColor, outPixels, tilegrid.width, tilegrid.height);
                break;
            }
        }

        uploadTilemapTextureRGBA(outPixels, tilegrid);
    }

    scale *= editor.editorZoom;

    const int trueSize = tileSize * scale;

    int trueWidth = tilegrid.width * scale;
    int trueHeight = tilegrid.height * scale;

    ImGui::BeginChild("LevelRegion", ImVec2(trueWidth, trueHeight), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    editor.inLevelRegion = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);

    DrawNearestImage(dl, tilegrid, ImVec2(trueWidth, trueHeight), ImVec2(0, 0), ImVec2(1, 1));

    bool hovering = ImGui::IsItemHovered();

    ImGui::EndChild();

    ImVec2 min = ImGui::GetItemRectMin();
    ImVec2 max = ImGui::GetItemRectMax();

    int atlasWidth = trueWidth / trueSize;
    int atlasHeight = trueHeight / trueSize;

    int tileX, tileY;
    GetTileUnderMouse(min, trueSize, tileX, tileY);

    if (editor.editMode == EM_Collision)
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

        if (editor.selectedTile >= 0 && hovering)
        {
            ImU32 previewColor = collisionTypes[editor.selectedTile].color;
            DrawCollisionPreview(dl, min, trueSize, tileX, tileY, previewColor);

            if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
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
    else if (editor.inLevelRegion && hovering)
    {
        int atlasW = editor.tileset.width / tileSize;
        int atlasX = editor.selectedTile % atlasW;
        int atlasY = editor.selectedTile / atlasW;

        DrawTilePreview(dl, min, trueSize, tileSize, tileX, tileY, atlasX, atlasY, editor.tileset);

        if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && (editor.selectedTile >= 0 || editor.paintMode))
        {
            if (editor.editMode == EM_Layer2)
                PaintTileBackground(editor.layer2TileData, tileX, tileY, atlasWidth, editor.paintMode, false);
            else if (editor.editMode == EM_Layer3)
                PaintTileBackground(editor.layer3TileData, tileX, tileY, atlasWidth, editor.paintMode, true);
            else
                PaintTileGeneric(tileX, tileY, atlasWidth, editor.paintMode);
        }
        if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
        {
            int index = tileY * atlasWidth + tileX;
            switch (editor.editMode)
            {
                case EM_Layer2:
                    if (index >= 0 && index < editor.layer2TileData.size())
                        editor.selectedTile = editor.layer2TileData[index].tileId + (256 * editor.layer2TileData[index].vramPage);
                    break;
                case EM_Layer3:
                    if (index >= 0 && index < editor.layer3TileData.size())
                        editor.selectedTile = editor.layer3TileData[index].tileId + (256 * editor.layer3TileData[index].vramPage);
                    break;
                case EM_Metatiles: {
                    const int tilesPerRow = 32;
                    const int metaTilesPerRow = tilesPerRow / 2;

                    int metaX = tileX / 2;
                    int metaY = tileY / 2;
                    int metaTileIndex = metaY * metaTilesPerRow + metaX;

                    if (metaTileIndex > 0 && metaTileIndex < editor.levelMetaTiles.size())
                    {
                        int localX = tileX % 2;
                        int localY = tileY % 2;
                        int localIndex = localY * 2 + localX;
                        editor.selectedTile = editor.levelMetaTiles[metaTileIndex].macroIndex[localIndex];
                    }
                    break;
                }      
                default:
                    break;
            }

        }
    }

    DrawGrid(dl, min, max, trueSize);

    if (editor.editMode == EM_Metatiles)
    {
        const int s = 32;
        const int ts = s * editor.editorZoom;
        ImU32 color = IM_COL32(160, 0, 160, 128);
        DrawGrid(dl, min, max, ts, color);
    }
}

void App::drawTileView()
{
    int tileSize = 8;
    float scale = 2.0f;

    switch (editor.tileViewMode)
    {
        case VM_Tileset:
            tileSize = 16;
            scale = 2.0f;
            break;
        case VM_Metatiles: {
            tileSize = 32;
            scale = 1.0f;
            break;
        }
        default:
            break;
    }

    if (editor.rebuildTileset || editor.tileset.tex == 0)
    {
        if (editor.tileset.tex != 0)
            glDeleteTextures(1, &editor.tileset.tex);

        std::vector<ColorRGBA> outPixels;

        ColorRGBA bgColor = editor.palettes[0][0];
        if (!editor.universalBGColor)
            bgColor.a = 0;

        switch (editor.tileViewMode)
        {
            case VM_Layer2:
                tileSize = 8;
                scale = 2.0f;
                renderTileMapToRGBA(editor.layer2TileMap, editor.levelTiles, editor.aniPalettes[editor.paletteIndex], bgColor, outPixels, editor.tileset.width, editor.tileset.height);
                break;
            case VM_Layer3:
                tileSize = 8;
                scale = 2.0f;
                renderTileMapToRGBA(editor.layer3TileMap, editor.layer3Tiles, editor.subPalettes[editor.subPaletteIndex], bgColor, outPixels, editor.tileset.width, editor.tileset.height);
                break;
            case VM_Tileset:
                tileSize = 16;
                scale = 2.0f;
                renderTileMapToRGBA(editor.levelTileMap, editor.levelTiles, editor.aniPalettes[editor.paletteIndex], bgColor, outPixels, editor.tileset.width, editor.tileset.height);
                break;
            case VM_Metatiles: {
                tileSize = 32;
                scale = 1.0f;
                uint8_t offset = editor.mode == 0 ? 0 : 2;
                renderMetaTileMapToRGBA(editor.levelMetaTiles, 16, editor.levelTiles, editor.aniPalettes, offset, bgColor, outPixels, editor.tileset.width, editor.tileset.height);
                editor.rebuildTileset = false;
                break;
            }
        }
        
        uploadTilemapTextureRGBA(outPixels, editor.tileset);
    }

    scale *= editor.tilesetZoom;

    int trueSize = tileSize * scale;
    
    ImDrawList* dl = ImGui::GetWindowDrawList();

    int trueWidth = editor.tileset.width * scale;
    int trueHeight = editor.tileset.height * scale;

    DrawNearestImage(dl, editor.tileset, ImVec2(trueWidth, trueHeight), ImVec2(0, 0), ImVec2(1, 1));

    ImVec2 min = ImGui::GetItemRectMin();
    ImVec2 max = ImGui::GetItemRectMax();

    int atlasWidth = trueWidth / trueSize;
    int atlasHeight = trueHeight / trueSize;

    int tileX, tileY;
    GetTileUnderMouse(min, trueSize, tileX, tileY);

    bool hovering = ImGui::IsItemHovered();

    if (hovering)
        DrawHoverHighlight(dl, min, trueSize, tileX, tileY);

    if (ImGui::IsItemClicked())
        SelectTileFromClick(tileX, tileY, atlasWidth);

    if (editor.selectedTile >= 0)
    {
        int selX = editor.selectedTile % atlasWidth;
        int selY = editor.selectedTile / atlasWidth;
        DrawSelectedOutline(dl, min, trueSize, selX, selY);

        if (tileSize < 32)
        {
            float u0 = (selX * trueSize) / float(trueWidth);
            float v0 = (selY * trueSize) / float(trueHeight);
            float u1 = ((selX + 1) * trueSize) / float(trueWidth);
            float v1 = ((selY + 1) * trueSize) / float(trueHeight);

            scale = 16.0f * editor.tilesetZoom;
            ImVec2 bigSize(tileSize * scale, tileSize * scale);

            ImGui::SameLine();
            ImGui::BeginGroup();
            ImGui::Text("Selected Tile");

            ImDrawList* dl2 = ImGui::GetWindowDrawList();

            DrawNearestImage(dl, editor.tileset, bigSize, ImVec2(u0, v0), ImVec2(u1,v1));

            ImVec2 bigMin = ImGui::GetItemRectMin();
            ImVec2 bigMax = ImGui::GetItemRectMax();

            DrawGrid(dl2, bigMin, bigMax, scale);

            Palette pal;
            size_t psize = 4;
            if (editor.editMode == EM_Layer3)
            {
                pal = editor.subPalettes[editor.subPaletteIndex];
                psize = 4;
            }
            else
            {
                pal = editor.aniPalettes[editor.paletteIndex];
                psize = pal.size();
            }

            for (int i = 0; i < psize; ++i)
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
                if (tileSize == 16)
                    PaintMacroTilePixel(editor.selectedTile, px, py);
                else
                    PaintTilePixel(editor.selectedTile, px, py);
            }

            ImGui::EndGroup();
        }
    }

    DrawGrid(dl, min, max, trueSize);
}

void App::drawLevelView()
{
    ImGui::Text("CONTROLS:");
    ImGui::Text("Left and Right Arrow keys Scroll left and right");
    ImGui::Text("CTRL+= and CTRL+- Zooms in and out");
    if (editor.lvlViewMode == LVM_Level)
    {
        ImGui::Text("Hold Left Click to Paint with the Selected Meta Tile");
        ImGui::Text("Right Click to grab the currently hovered Meta Tile");
    }
    else
    {
        ImGui::Text("Left Click to Selected an Object");
        ImGui::Text("Hold Left Click to Drag an Object");
        ImGui::Text("Left Click an Object in the List to jump to it");
        ImGui::Text("Right Click an Object in the List for context menu");
        ImGui::Text("WARNING: Insert Above/Below shifts all objects up/down so the first/last Object will be overwritten");
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
                editor.lvlViewMode = static_cast<LvlViewMode>(i);
                editor.rebuildView = true;
                editor.rebuildBackgrounds = true;
            }
            if (selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    static int currentScreen = -1;
    static TilemapTexture tileGrid;
    static TilemapTexture layer2;
    static TilemapTexture layer3;

    size_t si = editor.levelData.size();
    int numScreens = si / 64;
    int fullMetaWidth = numScreens * 8;

    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))  editor.currentScreen--;
    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) editor.currentScreen++;

    int maxScroll = std::max(0, numScreens - 2);
    editor.currentScreen = std::clamp(editor.currentScreen, 0, maxScroll);

    ImGui::SliderInt("Screens", &editor.currentScreen, 0, maxScroll);

    if (editor.lvlViewMode == LVM_Level)
    {
        if (drawScrollData(editor.scrollData, editor.currentScreen, true) && editor.currentScreen < si - 1)
            drawScrollData(editor.scrollData, editor.currentScreen + 1, false);
    }

    int windowX = editor.currentScreen * 8;

    if (tileGrid.tex == 0 || editor.currentScreen != currentScreen || editor.rebuildView)
    {
        editor.rebuildView = false;
        currentScreen = editor.currentScreen;

        std::vector<ColorRGBA> outPixels;

        if (tileGrid.tex != 0)
            glDeleteTextures(1, &tileGrid.tex);

        ColorRGBA bgColor = editor.palettes[0][0];
        if (!editor.universalBGColor)
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
    if (editor.lvlViewMode == LVM_Level && editor.mode && (layer2.tex == 0 || layer3.tex == 0 || editor.rebuildBackgrounds))
    {
        editor.rebuildBackgrounds = false;

        ColorRGBA bgColor = editor.palettes[0][0];
        if (!editor.universalBGColor)
            bgColor.a = 0;

        if (layer2.tex != 0)
            glDeleteTextures(1, &layer2.tex);

        if (layer3.tex != 0)
            glDeleteTextures(1, &layer3.tex);

        std::vector<ColorRGBA> outPixels;

        renderBGTileMapToRGBA(editor.layer2TileData, 32, editor.levelTiles, editor.aniPalettes, bgColor, outPixels, layer2.width, layer2.height);
        /*
        28 = 32x32 single screen mirroring
        29 = 64x32 vertical mirroring (horizontal scroll)
        2A = 32x64 horizontal mirroring (vertical scroll)
        30 = 32x32 single screen mirroring
        31 = 64x32 vertical mirroring (horizontal scroll)
        32 = 32x64 horizontal mirroring (vertical scroll)
        */
        if (editor.bgTilemapMirror[editor.currentScreenId].bg2_mode == 0x29)
        {
            editor.scrollLayer2Vertical = false;
            verticalMirroring(outPixels, layer2.width, layer2.height, false);
        }
        else if (editor.bgTilemapMirror[editor.currentScreenId].bg2_mode == 0x28)
        {
            if (editor.scrollLayer2Vertical)
                horizontalMirroring(outPixels, layer2.width, layer2.height);
            else
                verticalMirroring(outPixels, layer2.width, layer2.height, true);
        }
        else
        {
            editor.scrollLayer2Vertical = true;
        }

        uploadTilemapTextureRGBA(outPixels, layer2);
        outPixels.clear();
        renderBGTileMapToRGBA(editor.layer3TileData, 32, editor.layer3Tiles, editor.subPalettes, bgColor, outPixels, layer3.width, layer3.height);

        if (editor.bgTilemapMirror[editor.currentScreenId].bg3_mode == 0x31)
        {
            editor.scrollLayer3Vertical = false;
            verticalMirroring(outPixels, layer3.width, layer3.height, false);
        }
        else if (editor.bgTilemapMirror[editor.currentScreenId].bg3_mode == 0x30)
        {
            if (editor.scrollLayer2Vertical)
                horizontalMirroring(outPixels, layer3.width, layer3.height);
            else
                verticalMirroring(outPixels, layer3.width, layer3.height, true);
        }
        else
        {
            editor.scrollLayer3Vertical = true;
        }
        uploadTilemapTextureRGBA(outPixels, layer3);
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

    if (editor.lvlViewMode == LVM_Level)
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
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && editor.inLevelRegion)
                {
                    int worldTileX = tileX + windowX;
                    int worldTileY = tileY;

                    int index = worldTileY * fullMetaWidth + worldTileX;

                    if (index >= 0 && index < editor.levelData.size())
                    {
                        editor.selectedTile = editor.levelData[index];
                    }
                }
            }
        }

        if (editor.mode)
        {
            std::string label = "Layer 2 Scroll Screen ";
            ImGui::Checkbox((label + std::to_string(editor.currentScreen)).c_str(), &editor.bgScrollData[editor.currentScreen].bg2);
            ImGui::SameLine();
            ImGui::Checkbox((label + std::to_string(editor.currentScreen + 1)).c_str(), &editor.bgScrollData[editor.currentScreen + 1].bg2);

            label = "Layer 3 Scroll Screen ";
            ImGui::Checkbox((label + std::to_string(editor.currentScreen)).c_str(), &editor.bgScrollData[editor.currentScreen].bg3);
            ImGui::SameLine();
            ImGui::Checkbox((label + std::to_string(editor.currentScreen + 1)).c_str(), &editor.bgScrollData[editor.currentScreen + 1].bg3);

            drawBGScrollData();

            ImGui::Checkbox("Preview Scroll", &editor.previewScroll);

            trueWidth = layer2.width * editor.editorZoom;
            trueHeight = layer2.height * editor.editorZoom;

            if (editor.previewScroll)
            {
                if (!editor.scrollLayer2Vertical)
                {
                    DrawHorizontalWrappingImage(dl, layer2, trueWidth, trueHeight, editor.layer2Scanlines, editor.editorZoom);
                }
                else
                {
                    DrawVerticalWrappingImage(dl, layer2, trueWidth, trueHeight, editor.layer2Scanlines, editor.editorZoom);
                }
                trueWidth = layer3.width * editor.editorZoom;
                trueHeight = layer3.height * editor.editorZoom;
                if (!editor.scrollLayer3Vertical)
                {
                    DrawHorizontalWrappingImage(dl, layer3, trueWidth, trueHeight, editor.layer3Scanlines, editor.editorZoom);
                }
                else
                {
                    DrawVerticalWrappingImage(dl, layer3, trueWidth, trueHeight, editor.layer3Scanlines, editor.editorZoom);
                }
            }
            else
            {
                DrawNearestImage(dl, layer2,
                    ImVec2(trueWidth, trueHeight),
                    ImVec2(0, 0),
                    ImVec2(1, 1)
                );
                trueWidth = layer3.width * editor.editorZoom;
                trueHeight = layer3.height * editor.editorZoom;
                DrawNearestImage(dl, layer3,
                    ImVec2(trueWidth, trueHeight),
                    ImVec2(0, 0),
                    ImVec2(1, 1)
                );
            }
        }
    }
    else
    {
        int screenA = editor.currentScreen;
        int screenB = editor.currentScreen + 1;

        if (hovering && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !editor.dragging)
        {
            editor.selectedObject = -1;
            editor.objectType = -1;
        }

        ImGui::Begin("Object List");

        for (int type = 0; type < 2; ++type)
        {
            if (type == 0)
            {
                ImGui::BeginChild("EnemyList", ImVec2(250, 300), true);
                ImGui::SeparatorText("Enemies");
            }
            else
            {
                ImGui::BeginChild("ItemList", ImVec2(250, 300), true);
                ImGui::SeparatorText("Items");
            }
            Objects* objs = (type == 0 ? &editor.enemyData : &editor.itemData);
            int index = 0;

            for (size_t i = 0; i < objs->size(); ++i)
            {
                Object& obj = (*objs)[i];
                bool onScreen = (obj.screen == screenA || obj.screen == screenB);
                bool valid = (obj.type != 0xFF);
                bool selected = editor.selectedObject == i;
                if (ImGui::Selectable(std::format("{} {}",
                    (type == 0 ? "Enemy" : "Item"), i).c_str(), selected))
                {
                    editor.objectType = type;
                    editor.selectedObject = i;
                }

                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::MenuItem("Copy Object"))
                    {
                        editor.objClip.obj = obj;
                        editor.objClip.hasData = true;
                        editor.objClip.type = type;
                    }

                    if (ImGui::MenuItem("Paste Object", nullptr, false, editor.objClip.hasData && editor.objClip.type == type))
                    {
                        obj = editor.objClip.obj;
                    }

                    if (ImGui::MenuItem("Swap with Clipboard", nullptr, false, editor.objClip.hasData && editor.objClip.type == type))
                    {
                        std::swap(obj, editor.objClip.obj);
                    }

                    if (ImGui::MenuItem("Insert Blank Object Above", nullptr, false, i != 0))
                    {
                        for (size_t x = 1; x <= i; ++x)
                        {
                            (*objs)[x - 1] = (*objs)[x];
                        }
                        Object& o = (*objs)[i - 1];
                        o.screen = 255;
                        o.x = 255;
                        o.y = 255;
                        o.type = 255;
                    }

                    if (ImGui::MenuItem("Insert Blank Object Below", nullptr, false, i < objs->size() - 1))
                    {
                        for (size_t x = i + 1; x + 1 < objs->size(); ++x)
                        {
                            (*objs)[x + 1] = (*objs)[x];
                        }
                        Object& o = (*objs)[i + 1];
                        o.screen = 255;
                        o.x = 255;
                        o.y = 255;
                        o.type = 255;
                    }

                    if (ImGui::MenuItem("Delete Object"))
                    {
                        obj.screen = 255;
                    }

                    ImGui::EndPopup();
                }

                ImGui::BulletText("Screen: %u", obj.screen);
                ImGui::BulletText("X: %u", obj.x);
                ImGui::BulletText("Y: %u", obj.y);
                ImGui::BulletText("Type: %u", obj.type);
                ImGui::Separator();

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
            ImGui::EndChild();
            ImGui::SameLine();
        }
        ImGui::End();

        if (editor.selectedObject > -1)
        {
            Objects* objs =
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
            obj.screen = std::clamp(scr, 0, 255);
            obj.type = std::clamp(type, 0, 255);

            if (obj.screen > editor.currentScreen + 1 || obj.screen < editor.currentScreen)
            {
                editor.currentScreen = obj.screen;
                editor.rebuildView = true;
            }
        }
    }
}

bool App::drawScrollData(std::vector<uint8_t>& data, int screenNum, bool updateScreenId)
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
        label += std::to_string(index).c_str();
        if (ImGui::Checkbox(label.c_str(), &checked))
        {
            if (checked)
                flags |= item.bit;
            else
                flags &= ~item.bit;

            scrollByte = (scrollByte & 0x0F) | flags;
        }
        ImGui::SameLine();
    }
    ImGui::Text("");

    label = "Screen Amount " + std::to_string(index);

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

    if (updateScreenId)
        editor.currentScreenId = index;

    return screens - 1 == screenNum;
}

void App::drawBGScrollData()
{
    /*
        28 = 32x32 single screen mirroring
        29 = 64x32 vertical mirroring (horizontal scroll)
        2A = 32x64 horizontal mirroring (vertical scroll)
        30 = 32x32 single screen mirroring
        31 = 64x32 vertical mirroring (horizontal scroll)
        32 = 32x64 horizontal mirroring (vertical scroll)
        */
    constexpr FlagItem layer2Items[] = {
       { "32x32 Single Screen Sroll",  0x28 },
       { "64x32 Horizontal Scroll ",   0x29 },
       { "32x64 Vertical Scroll ",     0x2A }
    };

    constexpr FlagItem layer3Items[] = {
        { "32x32 Single Screen Scroll", 0x30 },
        { "64x32 Horizontal Scroll",    0x31 },
        { "32x64 Vertical Scroll",      0x32 }
    };

    BGTilemapMirror& tm = editor.bgTilemapMirror[editor.currentScreenId];
    std::string label = "Layer 2 Scrolling " + std::to_string(editor.currentScreenId);
    static int screenId = -1;
    static int layer2option = 0;
    static int layer3option = 0;
    if (screenId != editor.currentScreenId)
    {
        for (int i = 0; i < 3; ++i)
        {
            if (tm.bg2_mode == layer2Items[i].bit)
                layer2option = i;
            if (tm.bg3_mode == layer3Items[i].bit)
                layer3option = i;
        }
    }
    if (ImGui::BeginCombo(label.c_str(), layer2Items[layer2option].name))
    {
        for (int i = 0; i < 3; ++i)
        {
            bool selected = i == layer2option;

            if (ImGui::Selectable(layer2Items[i].name, selected))
            {
                layer2option = i;
                tm.bg2_mode = layer2Items[i].bit;
                editor.rebuildBackgrounds = true;
                if (i == 1)
                    editor.scrollLayer2Vertical = false;
                else if (i == 2)
                    editor.scrollLayer2Vertical = true;
                else
                    editor.scrollLayer2Vertical = false;
            }

            if (selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    label = "Layer 3 Scrolling " + std::to_string(editor.currentScreenId);
    if (ImGui::BeginCombo(label.c_str(), layer3Items[layer3option].name))
    {
        for (int i = 0; i < 3; ++i)
        {
            bool selected = i == layer3option;

            if (ImGui::Selectable(layer3Items[i].name, selected))
            {
                layer3option = i;
                tm.bg3_mode = layer3Items[i].bit;
                editor.rebuildBackgrounds = true;
                if (i == 1)
                    editor.scrollLayer3Vertical = false;
                else if (i == 2)
                    editor.scrollLayer3Vertical = true;
                else
                    editor.scrollLayer3Vertical = false;
            }

            if (selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    float newWidth = ImGui::GetContentRegionAvail().x * 0.20f;
    ImGui::PushItemWidth(newWidth);
    int temp = editor.bgScrollSpeeds[editor.currentScreenId][editor.scrollLayer2Vertical ? 2 : 0].scanlines;
    ImGui::InputInt("Layer 2 Scroll Speed", &temp);
    editor.bgScrollSpeeds[editor.currentScreenId][editor.scrollLayer2Vertical ? 2 : 0].scanlines = std::clamp(temp, 0, 255);

    ImGui::SameLine();

    temp = editor.bgScrollSpeeds[editor.currentScreenId][editor.scrollLayer2Vertical ? 2 : 0].frames;
    ImGui::InputInt("Layer 2 Wait Frames", &temp);
    editor.bgScrollSpeeds[editor.currentScreenId][editor.scrollLayer2Vertical ? 2 : 0].frames = std::clamp(temp, 0, 255);

    temp = editor.bgScrollSpeeds[editor.currentScreenId][editor.scrollLayer2Vertical ? 3 : 1].scanlines;
    ImGui::InputInt("Layer 3 Scroll Speed", &temp);
    editor.bgScrollSpeeds[editor.currentScreenId][editor.scrollLayer2Vertical ? 3 : 1].scanlines = std::clamp(temp, 0, 255);

    ImGui::SameLine();

    temp = editor.bgScrollSpeeds[editor.currentScreenId][editor.scrollLayer2Vertical ? 3 : 1].frames;
    ImGui::InputInt("Layer 3 Wait Frames", &temp);
    editor.bgScrollSpeeds[editor.currentScreenId][editor.scrollLayer2Vertical ? 3 : 1].frames = std::clamp(temp, 0, 255);
    ImGui::PopItemWidth();
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
        saveMetaTilesToROM(editor.rom, level.chip32x32, level.chip32x32_palette, level.collision, editor.levelMetaTiles);

    if (!editor.layer2TileData.empty())
        saveBackgroundTileData(editor.rom, level.bg_tilemap, editor.layer2TileData);

    if (!editor.layer3TileData.empty())
        saveBackgroundTileData(editor.rom, level.bg_tilemap + 0x1000, editor.layer3TileData);

    if (!editor.bgScrollData.empty())
        saveBackgroundScrollData(editor.rom, level.bg_scroll, editor.bgScrollData);

    if (!editor.bgScrollSpeeds.empty())
        saveBGScrollSpeeds(editor.rom, level.bg_speed, editor.bgScrollSpeeds);

    if (!editor.bgTilemapMirror.empty())
        saveBGTilemapMirror(editor.rom, level.bg_mirror, editor.bgTilemapMirror);

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
