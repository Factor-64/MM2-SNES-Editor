#include "level.h"

std::vector<uint8_t> loadLevelData(const std::vector<uint8_t>& rom, uint32_t addr, int count)
{
	return std::vector<uint8_t>(rom.begin() + addr, rom.begin() + addr + count);
}

std::vector<uint8_t> loadScrollData(const std::vector<uint8_t>& rom, uint32_t addr, int count)
{
    return std::vector<uint8_t>(rom.begin() + addr, rom.begin() + addr + count);
}

std::vector<uint8_t> remapColumnMajorScreensHorizontally(const std::vector<uint8_t>& levelData)
{
    const int screenW = 8;
    const int screenH = 8;
    const int tilesPerScreen = screenW * screenH;

    int numScreens = levelData.size() / tilesPerScreen;

    std::vector<uint8_t> out;
    out.resize(levelData.size());

    for (int s = 0; s < numScreens; ++s)
    {
        const int screenOffset = s * tilesPerScreen;

        for (int col = 0; col < screenW; ++col)
        {
            for (int row = 0; row < screenH; ++row)
            {
                int src = screenOffset + col * screenH + row;
                int dstRow = row;
                int dstCol = s * screenW + col;

                int dst = dstRow * (numScreens * screenW) + dstCol;

                out[dst] = levelData[src];
            }
        }
    }

    return out;
}

std::vector<uint8_t> remapBackToColumnMajor(const std::vector<uint8_t>& data)
{
    const int screenW = 8;
    const int screenH = 8;
    const int tilesPerScreen = screenW * screenH;

    int numScreens = data.size() / tilesPerScreen;

    std::vector<uint8_t> out(data.size());

    for (int s = 0; s < numScreens; ++s)
    {
        const int screenOffset = s * tilesPerScreen;

        for (int col = 0; col < screenW; ++col)
        {
            for (int row = 0; row < screenH; ++row)
            {
                int srcRow = row;
                int srcCol = s * screenW + col;

                int src = srcRow * (numScreens * screenW) + srcCol;

                int dst = screenOffset + col * screenH + row;

                out[dst] = data[src];
            }
        }
    }

    return out;
}

void saveLevelData(std::vector<uint8_t>& rom, uint32_t addr, const std::vector<uint8_t>& newData)
{
    auto data = remapBackToColumnMajor(newData);
    std::copy(data.begin(), data.end(), rom.begin() + addr);
}

void saveScrollData(std::vector<uint8_t>& rom, uint32_t addr, const std::vector<uint8_t>& data)
{
    std::copy(data.begin(), data.end(), rom.begin() + addr);
}

int metaWidthFromLevelData(const std::vector<uint8_t>& levelData)
{
    const int screenSize = 8 * 8;
    int numScreens = levelData.size() / screenSize;
    return numScreens * 8;
}


