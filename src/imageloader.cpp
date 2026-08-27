#include "imageloader.h"
#include "lodepng.h"
#include <fstream>
#include <iostream>

#pragma pack(push, 1)
struct BMPFileHeader {
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
};

struct BMPInfoHeader {
    uint32_t biSize;
    int32_t  biWidth;
    int32_t  biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t  biXPelsPerMeter;
    int32_t  biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
};

struct RGBQUAD {
    uint8_t blue;
    uint8_t green;
    uint8_t red;
    uint8_t reserved;
};
#pragma pack(pop)

bool loadIndexedBMP(const std::string& filename, Palette& palette, std::vector<uint8_t>& pixels, int& width, int& height) 
{
    std::ifstream file(filename, std::ios::binary);
    if (!file) return false;

    BMPFileHeader fh;
    BMPInfoHeader ih;

    file.read((char*)&fh, sizeof(fh));
    file.read((char*)&ih, sizeof(ih));

    if (fh.bfType != 0x4D42) return false; // "BM"
    if (ih.biCompression != 0) return false; // BI_RGB only

    if (!(ih.biBitCount == 1 || ih.biBitCount == 2 ||
        ih.biBitCount == 4 || ih.biBitCount == 8))
        return false;

    width = ih.biWidth;
    height = ih.biHeight;

    int paletteCount = ih.biClrUsed ? ih.biClrUsed : (1 << ih.biBitCount);

    // BMP stores BGRA palette
    for (int i = 0; i < 16; ++i) 
    {
        RGBQUAD color = { 0,0,0,0 };
        if (i < paletteCount)
            file.read((char*)&color, sizeof(color));

        palette[i].r = color.red;
        palette[i].g = color.green;
        palette[i].b = color.blue;
        palette[i].a = 255;
    }

    file.seekg(fh.bfOffBits, std::ios::beg);

    int bits = ih.biBitCount;
    int pixelsPerByte = 8 / bits;
    int rowBytes = (width + pixelsPerByte - 1) / pixelsPerByte;
    int paddedRowBytes = (rowBytes + 3) & ~3;

    pixels.resize(width * height);

    std::vector<uint8_t> row(paddedRowBytes);

    // BMP is bottom-up
    for (int y = height - 1; y >= 0; --y) 
    {
        file.read((char*)row.data(), paddedRowBytes);

        for (int x = 0; x < width; ++x) 
        {
            int byteIndex = x / pixelsPerByte;
            int shift = (pixelsPerByte - 1 - (x % pixelsPerByte)) * bits;

            uint8_t index = (row[byteIndex] >> shift) & ((1 << bits) - 1);
            pixels[y * width + x] = index;
        }
    }

    return true;
}

bool loadIndexedPNG(const std::string& filename, Palette& palette, std::vector<uint8_t>& pixels, int& width, int& height)
{
    std::vector<unsigned char> fileBuffer;
    unsigned error = lodepng::load_file(fileBuffer, filename);
    if (error) return false;

    unsigned char* out = nullptr;
    unsigned w, h;

    LodePNGState state;
    lodepng_state_init(&state);

    // decoder should keep indexed format
    state.decoder.color_convert = 0;

    error = lodepng_decode(&out, &w, &h, &state, fileBuffer.data(), fileBuffer.size());
    if (error) return false;

    if (state.info_png.color.colortype != LCT_PALETTE)
        return false;

    width = w;
    height = h;

    unsigned char* pngPalette = state.info_png.color.palette;
    unsigned paletteSize = state.info_png.color.palettesize;

    for (int i = 0; i < 16; ++i)
    {
        if (i < paletteSize)
        {
            palette[i].r = pngPalette[i * 4 + 0];
            palette[i].g = pngPalette[i * 4 + 1];
            palette[i].b = pngPalette[i * 4 + 2];
        }
    }

    pixels.assign(out, out + width * height);

    free(out);
    lodepng_state_cleanup(&state);

    return true;
}

std::vector<Tile> extractTiles(const std::vector<uint8_t>& pixels, int width, int height) 
{
    const int tileW = 8;
    const int tileH = 8;

    int tilesX = width / tileW;
    int tilesY = height / tileH;

    std::vector<Tile> tiles;
    tiles.reserve(tilesX * tilesY);

    for (int ty = 0; ty < tilesY; ++ty) 
    {
        for (int tx = 0; tx < tilesX; ++tx) 
        {
            Tile tile{};
            int idx = 0;

            for (int y = 0; y < tileH; ++y) 
            {
                for (int x = 0; x < tileW; ++x) 
                {
                    int px = (ty * tileH + y) * width + (tx * tileW + x);
                    uint8_t index = pixels[px] & 0x0F;

                    tile.pixels[idx++] = index;
                }
            }

            tiles.push_back(tile);
        }
    }

    return tiles;
}

std::vector<Tile> convert4bppTo2bpp(const std::vector<Tile>& tiles)
{
    std::vector<Tile> out = tiles;
    for (size_t i = 0; i < out.size(); ++i)
    {
        Tile& tile = out[i];
        for (int x = 0; x < tile.pixels.size(); ++x)
        {
            tile.pixels[x] = tile.pixels[x] & 0x03;
        }
    }
    return out;
}