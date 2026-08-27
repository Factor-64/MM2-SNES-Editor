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
#include "imageloader.h"
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
        case MS_ImportGraphics: {
            if (!ImGuiFileDialog::Instance()->IsOpened())
            {
                IGFD::FileDialogConfig config;
                config.countSelectionMax = 1;

                ImGuiFileDialog::Instance()->OpenDialog(
                    "ChooseIndexedImage",
                    "Select Index Image File",
                    "PNG (*.png){.png},BMP (*.bmp){.bmp}",
                    config
                );
            }
            menuState = MS_NULL;
            break;
        }
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
        case MS_OpenGraphics: {
            openGraphics = true;
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
                    else if (key == "ChooseIndexedImage")
                    {
                        std::string filename = ImGuiFileDialog::Instance()->GetCurrentFileName();

                        std::string ext;
                        {
                            size_t dot = filename.find_last_of('.');
                            if (dot != std::string::npos)
                                ext = filename.substr(dot + 1);
                        }

                        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

                        std::vector<uint8_t> pixels;
                        int width = 0, height = 0;
                        bool success = false;

                        if (ext == "png") 
                        {
                            success = loadIndexedPNG(path, editor.image.pal, pixels, width, height);
                        }
                        else if (ext == "bmp")
                        {
                            success = loadIndexedBMP(path, editor.image.pal, pixels, width, height);
                        }
                        if (success)
                        {
                            editor.image.tiles4bpp = extractTiles(pixels, width, height);
                            if(editor.mode == 0)
                                editor.image.tiles4bpp = convert4bppTo2bpp(editor.image.tiles4bpp);
                            else
                                editor.image.tiles2bpp = convert4bppTo2bpp(editor.image.tiles4bpp);
                            editor.image.reload = true;
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
        if (editor.romLoaded && editor.jsonLoaded && editor.paletteLoaded)
        {
            bool ctrl = ImGui::GetIO().KeyCtrl;
            int zoomDelta = 0;

            if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Equal)) zoomDelta = 1;
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
                case AW_Graphics:
                    editor.graphicsZoom += zoomDelta;
                    if (editor.graphicsZoom < 1)
                        editor.graphicsZoom = 1;
                    else if (editor.graphicsZoom > 8)
                        editor.graphicsZoom = 8;
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
            if (openGraphics)
                drawGraphicsWindow();
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
    ImGui::SetNextWindowSizeConstraints(
        ImVec2(320, 240),
        ImVec2(FLT_MAX, FLT_MAX)
    );
    ImGui::Begin("Tileset", &open, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::SeparatorText("CONTROLS");
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
        }
        ImGui::EndCombo();
    }

    const std::string& levelName = names[editor.selectedLevel];
    const LevelEntry& level = editor.data[editor.mode].levels.at(levelName);
    const auto& gfx = editor.data[editor.mode].gfx;
    auto levelGfx = gfx.at(levelName);

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
        }
        else
        {
            for (auto it = levelGfx.commonIdx.rbegin(); it != levelGfx.commonIdx.rend(); ++it)
            {
                size_t i = *it;
                if (i >= commonGfx.layer12.size())
                    throw std::runtime_error("commonIdx index out of range");

                levelGfx.layer12.insert(levelGfx.layer12.begin(), commonGfx.layer12[i]);
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

        editor.checkpointData = loadCheckpoints(editor.rom, level.midpoint_start_y);

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

        size_t si = editor.levelData.size();
        editor.screenCount = si / 64;
    }
    static std::string label = "Tileset for " + levelName;
    const char* tabNames[] = { "Tile Editor", "Level Editor", "Collision Editor", "Background Layer 2 Editor", "Background Layer 3 Editor"};
    if (ImGui::BeginCombo("Editor Mode", tabNames[editor.tileViewMode]))
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
                        label = "Tileset for " + levelName;
                        editor.editMode = EM_Metatiles;
                        break;
                    case VM_Metatiles:
                        label = "Meta Tiles for " + levelName;
                        editor.editMode = EM_Level;
                        break;
                    case VM_Collision:
                        editor.editMode = EM_Collision;
                        break;
                    case VM_Layer2:
                        label = "Tileset for " + levelName;
                        editor.editMode = EM_Layer2;
                        break;
                    case VM_Layer3:
                        label = "Tileset for " + levelName;
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
        }
        ImGui::EndCombo();
    }
    
    if (editor.tileViewMode != VM_Collision)
    {
        ImGui::SeparatorText(label.c_str());
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
            uint32_t base = level.palette_layer3;

            for (int i = 0; i < 16; ++i)
            {
                uint32_t addr = base + (paletteIndex * 16 + i) * 2;
                writeSNESColor(editor.rom, addr, pal[i]);
            }

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
    ImGui::SetNextWindowSizeConstraints(
        ImVec2(320, 240),
        ImVec2(FLT_MAX, FLT_MAX)
    );
    ImGui::Begin("Palettes", &open, ImGuiWindowFlags_HorizontalScrollbar);
    
    ImGui::SeparatorText("CONTROLS");
    ImGui::Text("Left Click a color to change it");
    ImGui::Text("Left Click a palette name to copy, paste, or swap with clipboard");
    ImGui::Text("Up/Down Arrow increase/decrease BG Palette Index");

    auto& names = editor.data[editor.mode].levelNames;
    const std::string& levelName = names[editor.selectedLevel];
    const LevelEntry& level = editor.data[editor.mode].levels.at(levelName);

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
            if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))  editor.subPaletteIndex--;
            if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) editor.subPaletteIndex++;
            if (editor.subPaletteIndex > 3)
                editor.subPaletteIndex = 3;
            else if (editor.subPaletteIndex < 0)
                editor.subPaletteIndex = 0;
            ImGui::Text("Left/Right Arrow decrease/increase Sub-Palette Index");
            break;
        default:
            break;
    }

    std::string label = "Palettes for ";
    label += levelName;

    ImGui::SeparatorText(label.c_str());

    if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))  editor.paletteIndex--;
    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) editor.paletteIndex++;

    if (editor.paletteIndex < offset)
        editor.paletteIndex = offset;
    else if (editor.paletteIndex > colorCount - 1)
        editor.paletteIndex = colorCount - 1;

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

inline void DrawTextOutlined(ImDrawList* dl, ImVec2 pos, ImU32 colText, const char* text, float zoom)
{
    ImGui::SetWindowFontScale(zoom);

    ImU32 colOutline = IM_COL32(255, 255, 255, 255);

    dl->AddText(ImVec2(pos.x - 1, pos.y), colOutline, text);
    dl->AddText(ImVec2(pos.x + 1, pos.y), colOutline, text);
    dl->AddText(ImVec2(pos.x, pos.y - 1), colOutline, text);
    dl->AddText(ImVec2(pos.x, pos.y + 1), colOutline, text);

    dl->AddText(pos, colText, text);
    ImGui::SetWindowFontScale(1);
}

void App::drawLevelWindow()
{
    ImGui::SetNextWindowSizeConstraints(
        ImVec2(320, 240),
        ImVec2(FLT_MAX, FLT_MAX)
    );
    ImGui::Begin("Editor", &open, ImGuiWindowFlags_HorizontalScrollbar);

    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootWindow))
    {
        editor.activeWindow = AW_Editor;
    }

    if (editor.editMode != EM_Level)
    {
        ImGui::SeparatorText("CONTROLS");
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
            ImGui::Text("Paint Mode makes Left Click only Paint with the currently Selected Palette & Attributes");
            ImGui::Checkbox("Paint Mode", &editor.paintMode);
            if (editor.editMode == EM_Layer2 || editor.editMode == EM_Layer3)
            {
                ImGui::Checkbox("Horizontal Flip", &editor.hFlip);
                ImGui::SameLine();
                ImGui::Checkbox("Vertical Flip", &editor.vFlip);
                ImGui::SameLine();
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

inline void DrawNearestImage(ImDrawList* dl, const TilemapTexture& tileset, const ImVec2& size, const ImVec2& c1, const ImVec2& c2, const ImVec2 pos = ImVec2(-1, -1))
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

void App::drawHeaderWindow()
{
    ImGui::SetNextWindowSizeConstraints(
        ImVec2(320, 240),
        ImVec2(FLT_MAX, FLT_MAX)
    );
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

inline void GetTileUnderMouse(const ImVec2& min, int w, int h, int& outX, int& outY)
{
    ImVec2 mouse = ImGui::GetMousePos();
    ImVec2 rel(mouse.x - min.x, mouse.y - min.y);

    outX = static_cast<int>(rel.x / w);
    outY = static_cast<int>(rel.y / h);
}

inline void DrawGrid(ImDrawList* dl, const ImVec2& min, const ImVec2& max, int w, int h, ImU32 color = IM_COL32(80, 80, 80, 128))
{
    int cols = (max.x - min.x) / w;
    int rows = (max.y - min.y) / h;

    for (int x = 0; x <= cols; ++x)
        dl->AddLine(ImVec2(min.x + x * w, min.y), ImVec2(min.x + x * w, max.y), color);

    for (int y = 0; y <= rows; ++y)
        dl->AddLine(ImVec2(min.x, min.y + y * h), ImVec2(max.x, min.y + y * h), color);
}

inline void DrawHoverHighlight(ImDrawList* dl, const ImVec2& min, int w, int h, int x, int y)
{
    float x0 = min.x + x * w;
    float y0 = min.y + y * h;
    float x1 = x0 + w;
    float y1 = y0 + h;

    dl->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y1), IM_COL32(100, 150, 255, 60));
    dl->AddRect(ImVec2(x0, y0), ImVec2(x1, y1), IM_COL32(100, 150, 255, 255), 0, 0, 2.0f);
}

inline void DrawSelectedOutline(ImDrawList* dl, const ImVec2& min, int w, int h, int x, int y, ImU32 color = IM_COL32(255, 255, 0, 40), bool outline = true)
{
    float x0 = min.x + x * w;
    float y0 = min.y + y * h;
    float x1 = x0 + w;
    float y1 = y0 + h;

    dl->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y1), color);
    if(outline)
        dl->AddRect(ImVec2(x0, y0), ImVec2(x1, y1), IM_COL32(255, 255, 0, 255), 0, 0, 2.0f);
}

inline void DrawTilePreview(ImDrawList* dl, const ImVec2& min, int ts, int s, int tileX, int tileY, int atlasX, int atlasY, const TilemapTexture& tileset)
{
    float x0 = min.x + tileX * ts;
    float y0 = min.y + tileY * ts;
    float x1 = x0 + ts;
    float y1 = y0 + ts;

    float u0 = (atlasX * s) / static_cast<float>(tileset.width);
    float v0 = (atlasY * s) / static_cast<float>(tileset.height);
    float u1 = ((atlasX + 1) * s) / static_cast<float>(tileset.width);
    float v1 = ((atlasY + 1) * s) / static_cast<float>(tileset.height);

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

inline void App::PaintTilePixel(int tileIndex, int x, int y, bool is2bpp)
{
    Tile& t = editor.levelTiles[tileIndex];

    y = y % 8;
    x = x % 8;

    t.pixels[y * 8 + x] = editor.selectedColor;

    saveTileToROM(t, editor.rom, is2bpp);

    editor.rebuildTileset = true;
    editor.rebuildEdit = true;
}

static void verticalMirroring(std::vector<ColorRGBA>& pixels, int& width, int& height, bool repeat)
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

static void horizontalMirroring(std::vector<ColorRGBA>& pixels, int& width, int& height)
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

static void DrawHorizontalWrappingImage(ImDrawList* dl, TilemapTexture& t, int& trueWidth, int& trueHeight, int& scanlines, int& zoom)
{
    int hW = trueWidth / 2;

    float uvOffset = fmod(static_cast<float>(scanlines * zoom) / t.width, 1.0f);
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
        int px0i = static_cast<int>(px0 + 0.5f);
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

static void DrawVerticalWrappingImage(ImDrawList* dl, TilemapTexture& t, int& trueWidth, int& trueHeight, int& scanlines, int& zoom)
{
    
    int hH = trueHeight / 2;

    float uvOffset = fmod(static_cast<float>(scanlines * zoom) / t.height, 1.0f);
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
        int px0i = static_cast<int>(px0 + 0.5f); 
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

inline void DrawUnSelectedBox()
{
    ImVec2 p0 = ImGui::GetCursorScreenPos();
    ImVec2 p1 = ImVec2(
        p0.x + ImGui::GetContentRegionAvail().x,
        p0.y + ImGui::GetTextLineHeight()
    );

    ImGui::GetWindowDrawList()->AddRectFilled(
        p0, p1,
        IM_COL32(32, 32, 32, 255)
    );
}

App::TileEditResult App::DrawTileEdit(TilemapTexture& tex, int& selX, int& selY, const int& tileW, const int& tileH, const float& scale, const Palette& pal, const int& psize, int& selectedColor, const int& trueWidth, const int& trueHeight)
{
    TileEditResult out{};

    float u0 = (selX * tileW) / float(trueWidth);
    float v0 = (selY * tileH) / float(trueHeight);
    float u1 = ((selX + 1) * tileW) / float(trueWidth);
    float v1 = ((selY + 1) * tileH) / float(trueHeight);

    ImVec2 bigSize(tileW * scale, tileH * scale);

    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::SeparatorText("Selected Tile");

    ImDrawList* dl = ImGui::GetWindowDrawList();
    DrawNearestImage(dl, tex, bigSize, ImVec2(u0, v0), ImVec2(u1, v1));

    ImVec2 bigMin = ImGui::GetItemRectMin();
    ImVec2 bigMax = ImGui::GetItemRectMax();

    DrawGrid(dl, bigMin, bigMax, scale * 2, scale * 2);

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
            selectedColor = i;

        if (selectedColor == i)
        {
            ImVec2 p0 = ImGui::GetItemRectMin();
            ImVec2 p1 = ImGui::GetItemRectMax();
            dl->AddRect(p0, p1, IM_COL32(255, 255, 0, 255), 0, 0, 2.0f);
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
        out.clicked = true;
        out.px = int((mouse.x - bigMin.x) / (scale * 2));
        out.py = int((mouse.y - bigMin.y) / (scale * 2));
    }

    ImGui::EndGroup();
    return out;
}

void HandleTileLink(int idx, bool prg, std::array<int, 1024>& linkFromTile, std::unordered_map<int, int>& linkFromImgTile, int& linkingTile, bool& linkingPRG)
{
    if (prg)
    {
        if (linkFromTile[idx] != -1)
        {
            int B = linkFromTile[idx];
            linkFromTile[idx] = -1;
            linkFromImgTile.erase(B);
            linkingTile = -1;
            return;
        }
    }
    else
    {
        if (linkFromImgTile.contains(idx))
        {
            int A = linkFromImgTile[idx];
            linkFromImgTile.erase(idx);
            linkFromTile[A] = -1;
            linkingTile = -1;
            return;
        }
    }

    if (linkingTile == -1)
    {
        linkingTile = idx;
        linkingPRG = prg;
        return;
    }

    int first = linkingTile;
    int second = idx;

    // If linking started from prgrom:
    // first = A, second = B
    // If linking started from image:
    // first = B, second = A
    int A = linkingPRG ? first : second;
    int B = linkingPRG ? second : first;

    // Remove old links on A
    if (linkFromTile[A] != -1)
    {
        int oldB = linkFromTile[A];
        linkFromImgTile.erase(oldB);
    }

    if (linkFromImgTile.contains(B))
    {
        int oldA = linkFromImgTile[B];
        linkFromTile[oldA] = -1;
    }

    linkFromTile[A] = B;
    linkFromImgTile[B] = A;

    linkingTile = -1;
}

void ClearAllLinks(std::array<int, 1024>& linkFromTile, std::unordered_map<int, int>& linkFromImgTile, int& linkingTile, bool& linkingPGR)
{
    linkFromTile.fill(-1);
    linkFromImgTile.clear();
    linkingTile = -1;
    linkingPGR = false;
}


ImU32 ColorFromIndex(int idx)
{
    float hue = static_cast<float>(idx) / 1024.0f;

    float r, g, b;
    ImGui::ColorConvertHSVtoRGB(hue, 1.0f, 1.0f, r, g, b);

    return IM_COL32(
        static_cast<uint8_t>(r * 255),
        static_cast<uint8_t>(g * 255),
        static_cast<uint8_t>(b * 255),
        60
    );
}


void App::drawGraphicsWindow()
{
    ImGui::SetNextWindowSizeConstraints(
        ImVec2(320, 240),
        ImVec2(FLT_MAX, FLT_MAX)
    );
    ImGui::Begin("Graphics Viewer", &openGraphics);
    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootWindow))
    {
        editor.activeWindow = AW_Graphics;
    }
    static Range range;
    static int tileDisplay = 1;
    static std::vector<Tile> tiles;
    static TilemapTexture tex;
    static uint8_t shape = 0;
    static bool rebuild = false;
    static bool imgPal = false;
    static bool imgClicked = false;
    static bool continueCopy = false;
    static int paletteIndex = 0;
    static int subPaletteIndex = 0;
    constexpr uint32_t range_max = 0x8000;
    constexpr uint8_t formatSize[] = { 16, 32 };
    constexpr uint8_t tileSize[][2] = { {8, 8}, {8, 16}, {16, 16} };

    static std::array<int, 1024> linkFromTile = [] {
        std::array<int, 1024> arr{};
        arr.fill(-1);
        return arr;
    }();
    static std::unordered_map<int, int> linkFromImgTile;
    static int linkingTile = -1;
    static bool linkingPRG = false;

    static int tileIdx = -1;

    ImGui::Text("Current Address:  ");

    ImGui::SameLine();

    if (continueCopy)
        ImGui::BeginDisabled();

    if (ImGui::Button("<<"))
        range.start = (range.start >= range_max) ? range.start - range_max : 0;
    ImGui::SameLine(0, 0);
    if (ImGui::Button("<"))
        range.start = (range.start >= formatSize[tileDisplay]) ? range.start - formatSize[tileDisplay] : 0;

    ImGui::SameLine(0, 0);
    float newWidth = ImGui::GetContentRegionAvail().x * 0.75f;
    ImGui::PushItemWidth(newWidth);
    char buf[7];
    snprintf(buf, sizeof(buf), "%06X", range.start);

    if (ImGui::InputText("##CA", buf, sizeof(buf),
        ImGuiInputTextFlags_CharsHexadecimal))
    {
        long value = strtol(buf, nullptr, 16);
        value = std::clamp(value, 0L, 0xFFFFFFL);
        range.start = static_cast<uint32_t>(value);
    }
    ImGui::PopItemWidth();
    ImGui::SameLine(0, 0);

    if (ImGui::Button(">"))
        range.start = std::min<uint32_t>(range.start + formatSize[tileDisplay], 0xFFFFFF);
    ImGui::SameLine(0, 0);
    if (ImGui::Button(">>"))
        range.start = std::min<uint32_t>(range.start + range_max, 0xFFFFFF);

    if (range.start > editor.rom.size() - range_max)
        range.start = editor.rom.size() - range_max;

    auto& names = editor.data[editor.mode].levelNames;
    const std::string& levelName = names[editor.selectedLevel];
    const LevelEntry& level = editor.data[editor.mode].levels.at(levelName);
    const auto& gfx = editor.data[editor.mode].gfx;
    auto levelGfx = gfx.at(levelName);
    auto commonGfx = gfx.at("common");
    ImGui::PushItemWidth(newWidth);
    ImGui::Text("Common Graphics:  ");

    ImGui::SameLine();
    if (ImGui::BeginCombo("##CG", buf))
    {
        for (size_t i = 0; i < commonGfx.layer12.size(); ++i)
        {
            bool selected = commonGfx.layer12[i].start == range.start;
            std::stringstream ss;
            ss << std::uppercase << std::hex << commonGfx.layer12[i].start;
            std::string hex_str = ss.str();
            if (ImGui::Selectable(hex_str.c_str(), selected))
            {
                range.start = commonGfx.layer12[i].start;
            }
        }
        ImGui::EndCombo();
    }
    ImGui::Text("Level Graphics:   ");

    ImGui::SameLine();
    if (ImGui::BeginCombo("##LG", buf))
    {
        for (size_t i = 0; i < levelGfx.layer12.size(); ++i)
        {
            bool selected = levelGfx.layer12[i].start == range.start;
            std::stringstream ss;
            ss << std::uppercase << std::hex << levelGfx.layer12[i].start;
            std::string hex_str = ss.str();
            if (ImGui::Selectable(hex_str.c_str(), selected))
            {
                range.start = levelGfx.layer12[i].start;
            }
        }
        ImGui::EndCombo();
    }

    if (continueCopy)
        ImGui::EndDisabled();
    ImGui::Text("Display:          ");

    ImGui::SameLine();

    const static char* displayList[] = { "8x8", "8x16", "16x16" };
    if (ImGui::BeginCombo("##DSP", displayList[shape]))
    {
        for (int i = 0; i < 3; ++i)
        {
            bool selected = shape == i;
            if (ImGui::Selectable(displayList[i], selected))
            {
                shape = i;
                rebuild = true;
            }
        }
        ImGui::EndCombo();
    }
    
    if (editor.mode)
    {
        ImGui::Text("Format:           ");

        ImGui::SameLine();

        const static char* formatList[] = { "2bpp", "4bpp" };
        if (ImGui::BeginCombo("##FL", formatList[tileDisplay]))
        {
            for (int i = 0; i < 2; ++i)
            {
                bool selected = tileDisplay == i;
                if (ImGui::Selectable(formatList[i], selected))
                {
                    tileDisplay = i;
                    rebuild = true;
                }
            }
            ImGui::EndCombo();
        }
    }
    else
    {
        if (tileDisplay != 1)
        {
            tileDisplay = 1;
            rebuild = true;
        }
    }

    const bool is2bpp = tileDisplay == 0;
    bool palChange = false;

    ImGui::Text("Palette Index:    ");
    ImGui::SameLine();

    if (ImGui::BeginCombo("##GPI", std::to_string(paletteIndex).c_str()))
    {
        int size = 8;
        if(editor.mode) 
            size = is2bpp ? 2 : 16;
        
        if (paletteIndex >= size)
            paletteIndex = 0;
        
        for (int i = 0; i < size; ++i)
        {
            bool selected = paletteIndex == i;
            if (ImGui::Selectable(std::to_string(i).c_str(), selected))
            {
                palChange = true;
                paletteIndex = i;
                rebuild = true;
            }
        }
        ImGui::EndCombo();
    }
    if (editor.mode)
    {
        ImGui::Text("Sub-Palette Index:");
        ImGui::SameLine();

        if (ImGui::BeginCombo("##SPI", std::to_string(subPaletteIndex).c_str()))
        {
            for (int i = 0; i < 4; ++i)
            {
                bool selected = subPaletteIndex == i;
                if (ImGui::Selectable(std::to_string(i).c_str(), selected))
                {
                    palChange = true;
                    subPaletteIndex = i;
                    rebuild = true;
                }
            }
            ImGui::EndCombo();
        }
    }

    //ImGui::Text("Imported Graphics will use the formats selected above");
    //ImGui::Text("Imported Graphics will start at the current address and go till the end of the .bmp");
    
    //ImGui::Text("The palette will replace the currently selected palette/sub-palette");
    //ImGui::Text("Only the first 16/4 colors of the .bmp file will be used");
    if (ImGui::Button("Load Indexed Image"))
    {
        if(menuState == MS_NULL)
            menuState = MS_ImportGraphics;
    }

    if (range.end != range.start + range_max || tex.tex == 0 || rebuild)
    {
        if(!palChange)
            ClearAllLinks(linkFromTile, linkFromImgTile, linkingTile, linkingPRG);
        editor.image.reload = rebuild;
        rebuild = false;
        range.end = range.start + range_max;
        int mapWidth = shape == 2 ? 8 : 16;
        tiles = decodeTileRange(range, editor.rom, formatSize[tileDisplay]);
        TileMap tilemap = makeTileMap(tiles, mapWidth, shape);

        std::vector<ColorRGBA> outPixels;

        if (tex.tex != 0)
            glDeleteTextures(1, &tex.tex);

        ColorRGBA bgColor;

        renderTileMapToRGBA(tilemap, tiles, imgPal ? editor.image.pal : editor.aniPalettes[paletteIndex], bgColor, outPixels, tex.width, tex.height);
        uploadTilemapTextureRGBA(outPixels, tex);
    }

    std::vector<Tile>& imgTiles = (is2bpp ? editor.image.tiles2bpp : editor.image.tiles4bpp);

    if (!imgTiles.empty())
    {
        if (ImGui::Checkbox("Use Image Palette", &imgPal))
            rebuild = true;

        ImGui::Text("Right click a PGR Tile and an Image tile to link them");

        if (editor.image.reload)
        {
            editor.image.reload = false;
            int mapWidth = shape == 2 ? 8 : 16;
            TileMap tilemap = makeTileMap(imgTiles, mapWidth, shape);

            std::vector<ColorRGBA> outPixels;

            if (editor.image.texture.tex != 0)
                glDeleteTextures(1, &editor.image.texture.tex);

            ColorRGBA bgColor;

            renderTileMapToRGBA(tilemap, imgTiles, imgPal ? editor.image.pal : editor.aniPalettes[paletteIndex], bgColor, outPixels, editor.image.texture.width, editor.image.texture.height);
            uploadTilemapTextureRGBA(outPixels, editor.image.texture);
        }
    }
    ImGui::PopItemWidth();

    int trueWidth = tex.width * editor.graphicsZoom;
    int trueHeight = tex.height * editor.graphicsZoom;

    ImGui::BeginChild("PGRGraphics", ImVec2(trueWidth, trueHeight), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    DrawNearestImage(dl, tex, ImVec2(trueWidth, trueHeight), ImVec2(0, 0), ImVec2(1, 1));

    ImVec2 min = ImGui::GetItemRectMin();
    ImVec2 max = ImGui::GetItemRectMax();

    bool hovering = ImGui::IsItemHovered();

    const int tileW = tileSize[shape][0] * editor.graphicsZoom;
    const int tileH = tileSize[shape][1] * editor.graphicsZoom;
    DrawGrid(dl, min, max, tileW, tileH);

    ImGui::EndChild();

    int atlasWidth = trueWidth / tileW;
    //int atlasHeight = trueHeight / tileH;

    if (hovering)
    {
        int tileX = -1, tileY = -1;
        GetTileUnderMouse(min, tileW, tileH, tileX, tileY);
        DrawHoverHighlight(dl, min, tileW, tileH, tileX, tileY);
        if (ImGui::IsItemClicked())
        {
            tileIdx = tileY * atlasWidth + tileX;
            imgClicked = false;
        }
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
        {
            int idx = tileY * atlasWidth + tileX;
            HandleTileLink(idx, true, linkFromTile, linkFromImgTile, linkingTile, linkingPRG);
        }
    }

    bool hoveringImg = false;

    if (editor.image.texture.tex != 0)
    {
        ImGui::SameLine();

        ImGui::BeginGroup();

        if (continueCopy)
            ImGui::BeginDisabled();
        
        std::string label = "<< Replace All Tiles    ";
        if (ImGui::Button(label.c_str(), ImVec2(200, 0)) || continueCopy)
        {

            static size_t remaining = 0;

            if (!continueCopy)
                remaining = imgTiles.size();

            size_t chunk = std::min<size_t>(1024, remaining);

            chunk = std::min<size_t>(chunk, tiles.size());

            size_t offset = imgTiles.size() - remaining;

            std::copy(imgTiles.begin() + offset, imgTiles.begin() + offset + chunk, tiles.begin());

            for (size_t i = 0; i < chunk; ++i)
            {
                tiles[i].romAddress = range.start + i * formatSize[tileDisplay];
                saveTileToROM(tiles[i], editor.rom, is2bpp);
            }

            remaining -= chunk;

            if (remaining == 0)
                continueCopy = false;
            else
            {
                continueCopy = true;
                range.start += chunk;
                if (range.start > editor.rom.size() - range_max)
                {
                    range.start = editor.rom.size() - range_max;
                    continueCopy = false;
                }
            }
            rebuild = true;
        }
        label = "<< Replace Linked Tiles ";
        if (ImGui::Button(label.c_str(), ImVec2(200, 0)))
        {
            for (int A = 0; A < 1024; ++A)
            {
                int B = linkFromTile[A];
                if (B < 0) continue;
                if (B >= static_cast<int>(imgTiles.size())) continue;
                tiles[A].pixels = imgTiles[B].pixels;
                saveTileToROM(tiles[A], editor.rom, is2bpp);
                rebuild = true;
            }
            if(rebuild)
                ClearAllLinks(linkFromTile, linkFromImgTile, linkingTile, linkingPRG);
        }

        if (continueCopy)
            ImGui::EndDisabled();
        
        if (is2bpp)
            label = "<< Replace Sub-Palette " + std::to_string(subPaletteIndex);
        else
            label = "<< Replace Palette " + std::to_string(paletteIndex) + "   ";
        if (ImGui::Button(label.c_str(), ImVec2(200, 0)))
        {
            if (paletteIndex < 2)
                editor.paletteType = PT_Layer3;
            else if (paletteIndex < 6 || paletteIndex > 7)
                editor.paletteType = PT_Normal;
            else
                editor.paletteType = PT_Layer2;
            if (is2bpp)
            {
                Palette pal = editor.aniPalettes[paletteIndex];
                int base = subPaletteIndex * 4;
                for (int i = 0; i < 4; ++i)
                    pal[base + i] = editor.image.pal[base + i];
                writeSNESPaletteToROM(paletteIndex, pal, level);
            }
            else
            {
                if (editor.mode)
                    writeSNESPaletteToROM(paletteIndex, editor.image.pal, level);
                else
                    writeNESPaletteToROM(paletteIndex, editor.image.pal, level);
            }
        }

        ImGui::EndGroup();

        ImGui::SameLine();
        int trueW = editor.image.texture.width * editor.graphicsZoom;
        int trueH = editor.image.texture.height * editor.graphicsZoom;
        ImGui::BeginChild("IndexedImage", ImVec2(trueW, trueH), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        DrawNearestImage(dl, editor.image.texture, ImVec2(trueW, trueH), ImVec2(0, 0), ImVec2(1, 1));
        ImDrawList* dl2 = ImGui::GetWindowDrawList();
        ImVec2 imin = ImGui::GetItemRectMin();
        ImVec2 imax = ImGui::GetItemRectMax();

        hoveringImg = ImGui::IsItemHovered();
        DrawGrid(dl2, imin, imax, tileW, tileH);
        int atlasW = trueW / tileW;

        if (hoveringImg)
        {
            int tileX = -1, tileY = -1;
            GetTileUnderMouse(imin, tileW, tileH, tileX, tileY);
            DrawHoverHighlight(dl2, imin, tileW, tileH, tileX, tileY);
            if (ImGui::IsItemClicked())
            {
                tileIdx = tileY * atlasW + tileX;
                imgClicked = true;
            }
            if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
            {
                int idx = tileY * atlasW + tileX;
                HandleTileLink(idx, false, linkFromTile, linkFromImgTile, linkingTile, linkingPRG);
            }
        }

        if (linkingTile != -1)
        {
            if (!linkingPRG)
            {
                int x = linkingTile % atlasW;
                int y = linkingTile / atlasW;

                DrawSelectedOutline(dl2, imin, tileW, tileH, x, y, IM_COL32(255, 255, 255, 128), false);
            }
            else
            {
                int x = linkingTile % atlasWidth;
                int y = linkingTile / atlasWidth;

                DrawSelectedOutline(dl, min, tileW, tileH, x, y, IM_COL32(255, 255, 255, 128), false);
            }
        }

        for (int A = 0; A < 1024; ++A)
        {
            if (linkFromTile[A] != -1)
            {
                int ax = A % atlasWidth;
                int ay = A / atlasWidth;
                ImU32 color = ColorFromIndex(A);
                DrawSelectedOutline(dl, min, tileW, tileH, ax, ay, color, false);
                
                ax = linkFromTile[A] % atlasW;
                ay = linkFromTile[A] / atlasW;
                DrawSelectedOutline(dl2, imin, tileW, tileH, ax, ay, color, false);
            }
        }

        if (imgClicked)
        {
            min = imin;
            atlasWidth = atlasW;
            trueWidth = trueW;
            trueHeight = trueH;
            dl = dl2;
        }
        ImGui::EndChild();
    }
    ImGui::SameLine();

    if (tileIdx >= 0)
    {
        //dl = ImGui::GetWindowDrawList();
        static int selectedColor = 0;
        int selX = tileIdx % atlasWidth;
        int selY = tileIdx / atlasWidth;
        DrawSelectedOutline(dl, min, tileW, tileH, selX, selY);
        Palette pal;
        int psize = 4;
        if (imgPal)
        {
            pal = editor.image.pal;
            if (!is2bpp)
                psize = pal.size();
        }
        else
        {
            if (is2bpp)
            {
                pal = editor.subPalettes[subPaletteIndex];
            }
            else
            {
                pal = editor.aniPalettes[paletteIndex];
                psize = pal.size();
            }
        }
        
        float scale = editor.graphicsZoom;
        if (shape == 2)
            scale *= 4;
        else
            scale *= 8;
        TileEditResult result = DrawTileEdit((imgClicked ? editor.image.texture : tex), selX, selY, tileW, tileH, scale, pal, psize, selectedColor, trueWidth, trueHeight);

        if (result.clicked)
        {
            int tempIdx = tileIdx;

            int blockX = result.px / 8;
            int blockY = result.py / 8;

            if (shape == 1)
                tempIdx += blockY;
            else if (shape == 2)
                tempIdx += blockX + blockY * 2;

            int localX = result.px % 8;
            int localY = result.py % 8;
            std::vector<Tile>* ts = hoveringImg ? &imgTiles : &tiles;
            Tile& t = ts->at(tempIdx);
            t.pixels[localY * 8 + localX] = selectedColor;

            saveTileToROM(t, editor.rom, is2bpp);
            rebuild = true;
        }
    }

    ImGui::End();
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
    int tileX = -1, tileY = -1;
    GetTileUnderMouse(min, trueSize, trueSize, tileX, tileY);
 
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

    DrawGrid(dl, min, max, trueSize, trueSize);

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
        editor.rebuildTileset = false;

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

    bool hovering = ImGui::IsItemHovered();

    if (hovering)
    {
        int tileX = -1, tileY = -1;
        GetTileUnderMouse(min, trueSize, trueSize, tileX, tileY);
        DrawHoverHighlight(dl, min, trueSize, trueSize, tileX, tileY);
        if (ImGui::IsItemClicked())
            SelectTileFromClick(tileX, tileY, atlasWidth);
    }

    if (editor.selectedTile >= 0)
    {
        int selX = editor.selectedTile % atlasWidth;
        int selY = editor.selectedTile / atlasWidth;
        DrawSelectedOutline(dl, min, trueSize, trueSize, selX, selY);

        if (tileSize < 32)
        {
            Palette pal;
            int psize = 4;
            bool is2bpp = editor.editMode == EM_Layer3;
            if (is2bpp)
            {
                pal = editor.subPalettes[editor.subPaletteIndex];
            }
            else
            {
                pal = editor.aniPalettes[editor.paletteIndex];
                psize = pal.size();
            }

            scale = 8.0f * editor.tilesetZoom;
            if (tileSize == 8)
                scale *= 2;
            TileEditResult result = DrawTileEdit(editor.tileset, selX, selY, trueSize, trueSize, scale, pal, psize, editor.selectedColor, trueWidth, trueHeight);

            if (result.clicked)
            {
                if (tileSize == 16)
                    PaintMacroTilePixel(editor.selectedTile, result.px, result.py);
                else
                    PaintTilePixel(editor.selectedTile, result.px, result.py, is2bpp);
            }
        }
    }

    DrawGrid(dl, min, max, trueSize, trueSize);
}

void App::drawLevelView()
{
    ImGui::SeparatorText("CONTROLS");
    ImGui::Text("Left and Right Arrow keys Scroll left and right");
    ImGui::Text("CTRL+= and CTRL+- Zooms in and out");

    int fullMetaWidth = editor.screenCount * 8;
    int maxScreens = editor.screenCount;

    switch (editor.lvlViewMode)
    {
        case LVM_Level:
            ImGui::Text("Hold Left Click to Paint with the Selected Meta Tile");
            ImGui::Text("Right Click to grab the currently hovered Meta Tile");
            maxScreens -= 2;
            break;
        case LVM_Objects:
            ImGui::Text("Left Click to Selected an Object");
            ImGui::Text("Hold Left Click to Drag an Object");
            ImGui::Text("Left Click an Object in the List to jump to it");
            ImGui::Text("Right Click an Object in the List for context menu");
            ImGui::Text("WARNING: Insert Above/Below shifts all objects up/down so the first/last Object will be overwritten");
            maxScreens -= 2;
            break;
        case LVM_Checkpoints:
            maxScreens -= 1;
            break;
        default:
            break;
    }
    ImGui::Separator();

    auto& names = editor.data[editor.mode].levelNames;
    const std::string& levelName = names[editor.selectedLevel];
    const LevelEntry& level = editor.data[editor.mode].levels.at(levelName);

    const char* cbNames[] = { "Layout Editor", "Object Editor", "Checkpoint Editor" };
    if (ImGui::BeginCombo("Editor Type", cbNames[editor.lvlViewMode]))
    {
        for (int i = 0; i < 3; ++i)
        {
            bool selected = (editor.lvlViewMode == i);
            if (ImGui::Selectable(cbNames[i], selected))
            {
                editor.lvlViewMode = static_cast<LvlViewMode>(i);
                editor.rebuildView = true;
                editor.rebuildBackgrounds = true;
            }
        }
        ImGui::EndCombo();
    }

    static int currentScreen = -1;
    static TilemapTexture tileGrid;
    static TilemapTexture layer2;
    static TilemapTexture layer3;

    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))  editor.currentScreen--;
    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) editor.currentScreen++;

    int maxScroll = std::max(0, maxScreens);
    editor.currentScreen = std::clamp(editor.currentScreen, 0, maxScroll);

    ImGui::SliderInt("Screens", &editor.currentScreen, 0, maxScroll);

    bool screenChange = editor.currentScreen != currentScreen;

    int windowX = editor.currentScreen * 8;

    if (tileGrid.tex == 0 || screenChange || editor.rebuildView)
    {
        editor.rebuildView = false;
        currentScreen = editor.currentScreen;

        int index = 0;
        int totalScreens = 0;

        while (index < editor.scrollData.size())
        {
            uint8_t scrollByte = editor.scrollData[index];
            totalScreens += (scrollByte & 0x0F) + 1;

            if (totalScreens > editor.currentScreen)
                break;

            ++index;
        }

        editor.currentScreenId = index;

        std::vector<ColorRGBA> outPixels;

        if (tileGrid.tex != 0)
            glDeleteTextures(1, &tileGrid.tex);

        ColorRGBA bgColor = editor.palettes[0][0];
        if (!editor.universalBGColor)
            bgColor.a = 0;

        int windowWidth = editor.lvlViewMode == LVM_Checkpoints ? 8 : 16;
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
    if (editor.lvlViewMode == LVM_Level)
    {
        if (editor.mode && (layer2.tex == 0 || layer3.tex == 0 || editor.rebuildBackgrounds))
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

        drawScrollData();
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
        DrawGrid(dl, min, max, ts, ts);

        if (editor.inLevelRegion)
        {
            int tileX = -1, tileY = -1;
            GetTileUnderMouse(min, ts, ts, tileX, tileY);
            
            if (hovering)
            {
                DrawHoverHighlight(dl, min, ts, ts, tileX, tileY);
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
    else if(editor.lvlViewMode == LVM_Objects)
    {
        int screenA = editor.currentScreen;
        int screenB = editor.currentScreen + 1;

        if (hovering && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !editor.dragging)
        {
            editor.selectedObject = -1;
            editor.objectType = -1;
        }

        ImGui::SetNextWindowSizeConstraints(
            ImVec2(320, 240),
            ImVec2(FLT_MAX, FLT_MAX)
        );
        ImGui::Begin("Object List");

        float halfWidth = ImGui::GetContentRegionAvail().x * 0.5f;
        ImVec2 childSize(halfWidth, 0);

        for (int type = 0; type < 2; ++type)
        {
            if (type == 0)
            {
                ImGui::BeginChild("EnemyList", childSize, true);
                ImGui::SeparatorText("Enemies");
            }
            else
            {
                ImGui::BeginChild("ItemList", childSize, true);
                ImGui::SeparatorText("Items");
            }
            Objects* objs = (type == 0 ? &editor.enemyData : &editor.itemData);
            int index = 0;

            for (size_t i = 0; i < objs->size(); ++i)
            {
                Object& obj = (*objs)[i];
                bool onScreen = (obj.screen == screenA || obj.screen == screenB);
                bool valid = (obj.type != 0xFF);
                bool selected = editor.selectedObject == i && type == editor.objectType;

                if (!selected)
                {
                    DrawUnSelectedBox();
                }

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

                float px = min.x + ((obj.screen - screenA) * 256 + obj.x) * editor.editorZoom;
                float py = min.y + (obj.y * editor.editorZoom);

                const int baseSize = 8;
                const int size = baseSize * editor.editorZoom;

                ImVec2 p0(px, py);
                ImVec2 p1(px + size, py + size);

                ImVec2 mouse = ImGui::GetMousePos();
                bool hovered =
                    mouse.x >= p0.x && mouse.x <= p1.x &&
                    mouse.y >= p0.y && mouse.y <= p1.y;

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
                    int newX = mouse.x - editor.dragOffset.x - min.x - (obj.screen - screenA) * 256;
                    int newY = mouse.y - editor.dragOffset.y - min.y;

                    obj.x = std::clamp(newX, 0, 255);
                    obj.y = std::clamp(newY, 0, 255);
                }

                if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
                    editor.dragging = false;

                bool isSelected = (editor.objectType == type && editor.selectedObject == index);

                ImU32 col = isSelected
                    ? IM_COL32(255, 255, 0, 255)
                    : (type == 0 ? IM_COL32(255, 0, 0, 255) : IM_COL32(0, 0, 255, 255));

                float outline = 2.0f * editor.editorZoom;
                dl->AddRect(p0, p1, col, 0.0f, 0, outline);

                char buf[8];
                snprintf(buf, sizeof(buf), "%02d", obj.type);
                DrawTextOutlined(dl, ImVec2(px, py), IM_COL32(0, 0, 0, 255), buf, editor.editorZoom);

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

            if(ImGui::InputInt("X", &x))
                obj.x = std::clamp(x, 0, 255);
            if(ImGui::InputInt("Y", &y))
                obj.y = std::clamp(y, 0, 255);
            if(ImGui::InputInt("Screen #", &scr))
                obj.screen = std::clamp(scr, 0, 255);
            if(ImGui::InputInt("Type", &type))
                obj.type = std::clamp(type, 0, 255);

            if (obj.screen > editor.currentScreen + 1 || obj.screen < editor.currentScreen)
            {
                editor.currentScreen = obj.screen;
                editor.rebuildView = true;
            }
        }
    }
    else
    {
        static bool calc = true;
        ImGui::Checkbox("Auto Calculate Data Based on current screen", &calc);
        for (int i = 0; i < editor.checkpointData.size(); ++i)
        {
            Checkpoint* c = &editor.checkpointData[i];
            bool selected = c->screen == editor.currentScreen;

            if (selected)
            {
                std::string id = "##" + std::to_string(i);
                std::string label = "Checkpoint " + std::to_string(i);
                ImGui::SeparatorText(label.c_str());
                float newWidth = ImGui::GetContentRegionAvail().x * 0.50f;
                ImGui::PushItemWidth(newWidth);
                int screen = c->screen;
                label = "Screen" + id;
                if(ImGui::InputInt(label.c_str(), &screen))
                    screen = std::clamp(screen, 0, 255);
                int temp = c->y;
                label = "Y" + id;
                if (ImGui::InputInt(label.c_str(), &temp, 16))
                    c->y = std::clamp(temp, 4, 244);
                if (c->screen != screen)
                {
                    c->screen = screen;
                    editor.currentScreen = c->screen;
                    if (calc)
                    {
                        c->item_index = 0;
                        c->enemy_index = 0;
                        for (int x = 0; x < editor.enemyData.size(); ++x)
                        {
                            Object& o = editor.enemyData[x];
                            if (o.screen >= screen)
                            {
                                c->enemy_index = x;
                                break;
                            }
                        }
                        for (int x = 0; x < editor.itemData.size(); ++x)
                        {
                            Object& o = editor.itemData[x];
                            if (o.screen >= screen)
                            {
                                c->item_index = x;
                                break;
                            }
                        }
                        int index = 0;
                        int totalScreens = 0;
                        uint8_t scrollByte = 0;
                        bool previousScreenWouldMoveIndex = false;
                        while (index < editor.scrollData.size())
                        {
                            scrollByte = editor.scrollData[index];
                            totalScreens += (scrollByte & 0x0F) + 1;

                            if (c->screen + 1 <= totalScreens)
                                break;

                            ++index;
                        }
                        c->scroll = index;
                            
                        c->left_screen = c->screen;
                        c->right_screen = totalScreens - 1;
                        c->map_back_addr = 0x84E0 + (c->screen * 64);
                        c->map_forward_addr = c->map_back_addr + 128;
                    }
                }
                if (calc)
                    ImGui::BeginDisabled();

                label = "Left Scroll Screen" + id;
                temp = c->left_screen;
                if (ImGui::InputInt(label.c_str(), &temp))
                    c->left_screen = std::clamp(temp, 0, maxScreens);

                label = "Right Scroll Screen" + id;
                temp = c->right_screen;
                if (ImGui::InputInt(label.c_str(), &temp))
                    c->right_screen = std::clamp(temp, 0, maxScreens);

                label = "Scroll Data Index" + id;
                temp = c->scroll;
                if (ImGui::InputInt(label.c_str(), &temp))
                    c->scroll = std::clamp(temp, 0, static_cast<int>(editor.scrollData.size()));
                
                label = "Enemy Index" + id;
                temp = c->enemy_index;
                if (ImGui::InputInt(label.c_str(), &temp))
                    c->enemy_index = std::clamp(temp, 0, static_cast<int>(editor.enemyData.size()));
                
                label = "Item Index" + id;
                temp = c->item_index;
                if (ImGui::InputInt(label.c_str(), &temp))
                    c->item_index = std::clamp(temp, 0, static_cast<int>(editor.itemData.size()));

                {
                    char bufBack[5];
                    snprintf(bufBack, sizeof(bufBack), "%04X", c->map_back_addr);

                    label = "Level Backward Address" + id;
                    if (ImGui::InputText(label.c_str(), bufBack, sizeof(bufBack),
                        ImGuiInputTextFlags_CharsHexadecimal))
                    {
                        c->map_back_addr = std::clamp(static_cast<int>(strtol(bufBack, nullptr, 16)), 0, 0xFFFF);
                    }
                }

                {
                    char bufForward[5];
                    snprintf(bufForward, sizeof(bufForward), "%04X", c->map_forward_addr);

                    label = "Level Forward Address" + id;
                    if (ImGui::InputText(label.c_str(), bufForward, sizeof(bufForward),
                        ImGuiInputTextFlags_CharsHexadecimal))
                    {
                        c->map_forward_addr = std::clamp(static_cast<int>(strtol(bufForward, nullptr, 16)), 0, 0xFFFF);
                    }
                }

                if (calc)
                    ImGui::EndDisabled();

                float centerX = (min.x + max.x) * 0.5f;
                float y = min.y + static_cast<float>(c->y * editor.editorZoom);
                float halfW = 8.0f * editor.editorZoom;
                float halfH = 12.0f * editor.editorZoom;

                ImVec2 rectMin(floorf(centerX - halfW), floorf(y - halfH));
                ImVec2 rectMax(floorf(centerX + halfW), floorf(y + halfH));

                dl->AddRectFilled(rectMin, rectMax, IM_COL32(255, 255, 255, 255));
                dl->AddRect(rectMin, rectMax, IM_COL32(0, 0, 0, 255), 0.0f, 0, 1.0f);

                BGPositionData* bg = &editor.bgPositionData[i];
                label = "Layer 2 Position Data" + id;
                ImGui::SeparatorText(label.c_str());

                label = "Layer 2 X" + id;
                temp = bg->bg2_x;
                if (ImGui::InputInt(label.c_str(), &temp))
                    bg->bg2_x = std::clamp(temp, 0, 0xFFFF);

                label = "Layer 2 Y" + id;
                temp = bg->bg2_y;
                if (ImGui::InputInt(label.c_str(), &temp))
                    bg->bg2_y = std::clamp(temp, 0, 0xFFFF);

                label = "Layer 2 Screen ID" + id;
                temp = bg->bg2_screenId;
                if (ImGui::InputInt(label.c_str(), &temp))
                    bg->bg2_screenId = std::clamp(temp, 0, 0xFF);

                label = "Layer 3 Position Data" + id;
                ImGui::SeparatorText(label.c_str());

                label = "Layer 3 X" + id;
                temp = bg->bg3_x;
                if (ImGui::InputInt(label.c_str(), &temp))
                    bg->bg3_x = std::clamp(temp, 0, 0xFFFF);

                label = "Layer 3 Y" + id;
                temp = bg->bg3_y;
                if (ImGui::InputInt(label.c_str(), &temp))
                    bg->bg3_y = std::clamp(temp, 0, 0xFFFF);

                label = "Screen ID" + id;
                temp = bg->scrollId;
                if (ImGui::InputInt(label.c_str(), &temp))
                    bg->scrollId = std::clamp(temp, 0, 0xFF);
            }
        }
        ImGui::SetNextWindowSizeConstraints(
            ImVec2(320, 240),
            ImVec2(FLT_MAX, FLT_MAX)
        );
        ImGui::Begin("Checkpoint List");
        ImGui::SeparatorText("Checkpoints");
        for (int i = 0; i < editor.checkpointData.size(); ++i)
        {
            Checkpoint* c = &editor.checkpointData[i];
            bool selected = c->screen == editor.currentScreen;
            if (!selected)
                DrawUnSelectedBox();
            if (ImGui::Selectable(std::format("{} {}", "Checkpoint", i).c_str(), selected))
            {
                editor.currentScreen = c->screen;
            }
            
            ImGui::BulletText("Screen: %u", c->screen);
            ImGui::BulletText("Left Scroll Screen: %u", c->left_screen);
            ImGui::BulletText("Right Scroll Screen: %u", c->right_screen);
            ImGui::BulletText("Enemy Index: %u", c->enemy_index);
            ImGui::BulletText("Item Index: %u", c->item_index);
            {
                char bufBack[5];
                snprintf(bufBack, sizeof(bufBack), "%04X", c->map_back_addr);
                ImGui::BulletText("Level Backward Address: %s", bufBack);
            }
            {
                char bufForward[5];
                snprintf(bufForward, sizeof(bufForward), "%04X", c->map_forward_addr);
                ImGui::BulletText("Level Forward Address: %s", bufForward);
            }
            ImGui::Separator();
        }
        ImGui::End();

        ImGui::Begin("Background Position List");
        ImGui::SeparatorText("Background Position Data");
        for (int i = 0; i < editor.bgPositionData.size(); ++i)
        {
            BGPositionData* bg = &editor.bgPositionData[i];
            Checkpoint* c = &editor.checkpointData[i];
            bool selected = c->screen == editor.currentScreen;
            if (!selected)
                DrawUnSelectedBox();
            if (ImGui::Selectable(std::format("{} {}", "Position", i).c_str(), selected))
            {
                editor.currentScreen = c->screen;
            }
            ImGui::Text("Layer 2");
            ImGui::BulletText("X: %u", bg->bg2_x);
            ImGui::BulletText("Y: %u", bg->bg2_y);
            ImGui::BulletText("Screen ID: %u", bg->bg2_screenId);
            ImGui::Text("Layer 3");
            ImGui::BulletText("X: %u", bg->bg3_x);
            ImGui::BulletText("Y: %u", bg->bg3_y);
            ImGui::BulletText("Screen ID: %u", bg->scrollId);
            
            ImGui::Separator();
        }
        ImGui::End();
    }
}

void App::drawScrollData()
{
    uint8_t scrollByte = editor.scrollData[editor.currentScreenId];
    uint8_t flags = scrollByte & 0xF0;
    uint8_t screens = (scrollByte & 0x0F) + 1;

    constexpr uint8_t items[] = { 0x10, 0x20, 0x40, 0x80 };

    static const std::map<uint8_t, std::string> scrollMap = {
        { 0x10, "Scroll Right" },
        { 0x20, "Right Edge"   },
        { 0x40, "Bottom Edge"  },
        { 0x80, "Top Edge "    }
    };

    std::string label = "Screen Transition Type ID: " + std::to_string(editor.currentScreenId);

    ImGui::SeparatorText(label.c_str());

    for (const uint8_t& item : items)
    {
        bool checked = flags & item;

        label = scrollMap.at(item);
        label += " ##";
        label += std::to_string(editor.currentScreenId).c_str();
        if (ImGui::Checkbox(label.c_str(), &checked))
        {
            if (checked)
                flags |= item;
            else
                flags &= ~item;

            scrollByte = (scrollByte & 0x0F) | flags;
        }
        ImGui::SameLine();
    }
    ImGui::Text("");

    label = "Screen Amount ##" + std::to_string(editor.currentScreenId);

    if (ImGui::BeginCombo(label.c_str(), std::to_string(screens).c_str()))
    {
        for (int i = 1; i <= 16; ++i)
        {
            bool selected = (screens == i);
            if (ImGui::Selectable(std::to_string(i).c_str(), selected))
            {
                screens = i;
                scrollByte = (scrollByte & 0xF0) | ((screens - 1) & 0x0F);
            }
        }
        ImGui::EndCombo();
    }

    editor.scrollData[editor.currentScreenId] = scrollByte;

    ImGui::SetNextWindowSizeConstraints(
        ImVec2(320, 240),
        ImVec2(FLT_MAX, FLT_MAX)
    );
    ImGui::Begin("Scroll Data List");
    ImGui::SeparatorText("Level Screen ID Data");
    for (size_t i = 0; i < editor.scrollData.size(); ++i)
    {
        bool selected = editor.currentScreenId == i;

        if (!selected)
        {
            DrawUnSelectedBox();
        }

        if (ImGui::Selectable(std::format("{} {}", "Screen ID", i).c_str(), selected))
        {
            int index = 0;
            int totalScreens = 0;

            while (index < i && index < editor.scrollData.size())
            {
                uint8_t scrollByte = editor.scrollData[index];
                totalScreens += (scrollByte & 0x0F) + 1;

                ++index;
            }
            editor.currentScreen = totalScreens;
            editor.currentScreenId = index;
        }
        uint8_t sb = editor.scrollData[i];
        uint8_t f = sb & 0xF0;
        uint8_t s = (sb & 0x0F) + 1;
        std::string l = "";

        for (uint8_t bit = 4; bit < 8; ++bit) 
        {
            uint8_t data = 1 << bit;
            if (f & data) 
                l += scrollMap.at(data) + ", ";
        }
        if (l.size() > 2)
            l.erase(l.size() - 2);
        else
            l = "No Scroll";

        ImGui::BulletText("Scroll Types: %s", l.c_str());
        ImGui::BulletText("Screens Amount: %u", s);
        ImGui::Separator();
    }
    ImGui::End();
}

static void DrawBGScrollOptions(const char* label, BGSpeedData& data, float width, const char* idSuffix)
{
    ImGui::Text(label);
    ImGui::PushItemWidth(width);

    int temp = data.scanlines;
    if(ImGui::InputInt(std::string("Scroll Speed ").append(idSuffix).c_str(), &temp))
        data.scanlines = std::clamp(temp, 0, 255);

    ImGui::SameLine();

    temp = data.frames;
    if(ImGui::InputInt(std::string("Wait Frames ").append(idSuffix).c_str(), &temp))
        data.frames = std::clamp(temp, 0, 255);

    ImGui::PopItemWidth();
}

void App::drawBGScrollData()
{
    constexpr std::array<uint8_t, 3> layer2Items[] = { 0x28, 0x29, 0x2A };
    constexpr std::array<uint8_t, 3> layer3Items[] = { 0x30, 0x31, 0x32 };

    static const std::map<uint8_t, std::string> layer2Map = {
        { 0x28, "32x32 Single Screen Scroll" },
        { 0x29, "64x32 Horizontal Scroll" },
        { 0x2A, "32x64 Vertical Scroll" }
    };

    static const std::map<uint8_t, std::string> layer3Map = {
        { 0x30, "32x32 Single Screen Scroll" },
        { 0x31, "64x32 Horizontal Scroll" },
        { 0x32, "32x64 Vertical Scroll" }
    };

    std::string label = "Background Layer 2 ID: " + std::to_string(editor.currentScreenId);
    ImGui::SeparatorText(label.c_str());

    BGTilemapMirror& tm = editor.bgTilemapMirror[editor.currentScreenId];
    static int screenId = -1;

    if (ImGui::BeginCombo("Mirroring ##04", layer2Map.at(tm.bg2_mode).c_str()))
    {
        for (int i = 0; i < layer2Items->size(); ++i)
        {
            uint8_t type = layer2Items->at(i);
            bool selected = type == tm.bg2_mode;

            if (ImGui::Selectable(layer2Map.at(type).c_str(), selected))
            {
                tm.bg2_mode = type;
                editor.rebuildBackgrounds = true;
                if (i == 2)
                    editor.scrollLayer2Vertical = true;
                else
                    editor.scrollLayer2Vertical = false;
            }
        }
        ImGui::EndCombo();
    }

    float width = ImGui::GetContentRegionAvail().x * 0.20f;

    DrawBGScrollOptions("Horizontal Data", editor.bgScrollSpeeds[editor.currentScreenId][0], width, "##00");
    DrawBGScrollOptions("Vertical Data", editor.bgScrollSpeeds[editor.currentScreenId][2], width, "##01");

    label = "Background Layer 3 ID: " + std::to_string(editor.currentScreenId);

    ImGui::SeparatorText(label.c_str());

    if (ImGui::BeginCombo("Mirroring ##05", layer3Map.at(tm.bg3_mode).c_str()))
    {
        for (int i = 0; i < layer3Items->size(); ++i)
        {
            uint8_t type = layer3Items->at(i);
            bool selected = type == tm.bg3_mode;

            if (ImGui::Selectable(layer3Map.at(type).c_str(), selected))
            {
                tm.bg3_mode = type;
                editor.rebuildBackgrounds = true;
                if (i == 2)
                    editor.scrollLayer2Vertical = true;
                else
                    editor.scrollLayer2Vertical = false;
            }
        }
        ImGui::EndCombo();
    }

    DrawBGScrollOptions("Horizontal Data", editor.bgScrollSpeeds[editor.currentScreenId][1], width, "##02");
    DrawBGScrollOptions("Vertical Data", editor.bgScrollSpeeds[editor.currentScreenId][3], width, "##03");

    ImGui::SetNextWindowSizeConstraints(
        ImVec2(320, 240),
        ImVec2(FLT_MAX, FLT_MAX)
    );
    ImGui::Begin("Background Data List");
    float halfWidth = ImGui::GetContentRegionAvail().x * 0.5f;
    ImVec2 childSize(halfWidth, 0);
    ImGui::BeginChild("BackgroundScreenIDData", childSize, true);
    ImGui::SeparatorText("Background Screen ID Data");
    for (size_t i = 0; i < editor.bgScrollSpeeds.size(); ++i)
    {
        bool selected = editor.currentScreenId == i;

        if (!selected)
        {
            DrawUnSelectedBox();
        }

        if (ImGui::Selectable(std::format("{} {}","Screen ID", i).c_str(), selected))
        {
            int index = 0;
            int totalScreens = 0;

            while (index < i && index < editor.scrollData.size())
            {
                uint8_t scrollByte = editor.scrollData[index];
                totalScreens += (scrollByte & 0x0F) + 1;

                ++index;
            }
            editor.currentScreen = totalScreens;
            editor.currentScreenId = index;
        }

        BGSpeedData* sd = &editor.bgScrollSpeeds[i][0];
        ImGui::Text("Layer 2");
        ImGui::BulletText("Horizontal Speed: %u", sd->scanlines);
        ImGui::BulletText("Horizontal Wait Frames: %u", sd->frames);
        sd = &editor.bgScrollSpeeds[i][2];
        ImGui::BulletText("Vertical Speed: %u", sd->scanlines);
        ImGui::BulletText("Vertical Wait Frames: %u", sd->frames);

        BGTilemapMirror* tmm = &editor.bgTilemapMirror[i];
        ImGui::BulletText(layer2Map.at(tmm->bg2_mode).c_str());

        sd = &editor.bgScrollSpeeds[i][1];
        ImGui::Text("Layer 3");
        ImGui::BulletText("Horizontal Speed: %u", sd->scanlines);
        ImGui::BulletText("Horizontal Wait Frames: %u", sd->frames);
        sd = &editor.bgScrollSpeeds[i][3];
        ImGui::BulletText("Vertical Speed: %u", sd->scanlines);
        ImGui::BulletText("Vertical Wait Frames: %u", sd->frames);

        ImGui::BulletText(layer3Map.at(tmm->bg3_mode).c_str());
        
        ImGui::Separator();
    }
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("BackgroundScrollScreenData", childSize, true);
    ImGui::SeparatorText("Background Screen Data");
    for (size_t i = 0; i < editor.bgScrollData.size(); ++i)
    {
        bool selected = editor.currentScreen == i;

        if (!selected)
        {
            DrawUnSelectedBox();
        }

        if (ImGui::Selectable(std::format("{} {}", "Screen", i).c_str(), selected))
        {
            editor.currentScreen = i;
        }
        ScrollEnable& se = editor.bgScrollData[i];
        ImGui::BulletText("Layer 2 Scroll: %s", se.bg2 ? "Enabled" : "Disabled");
        ImGui::BulletText("Layer 3 Scroll: %s", se.bg3 ? "Enabled" : "Disabled");
        ImGui::Separator();
    }
    ImGui::EndChild();
    ImGui::End();
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

    if (!editor.checkpointData.empty())
        saveCheckpoints(editor.rom, level.midpoint_start_y, editor.checkpointData);

    if (!editor.bgPositionData.empty())
    {
        saveBGPositionData(editor.rom, level.bg_start, editor.bgPositionData[0]);
        saveBGPositionData(editor.rom, level.bg_checkpoint, editor.bgPositionData[1]);
        saveBGPositionData(editor.rom, level.bg_boss, editor.bgPositionData[2]);
    }
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
