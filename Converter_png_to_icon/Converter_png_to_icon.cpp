#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdint>
#include <filesystem>
#include <Windows.h>

namespace fs = std::filesystem;

#pragma pack(push, 1)
struct ICONDIRENTRY {
    uint8_t  bWidth;
    uint8_t  bHeight;
    uint8_t  bColorCount;
    uint8_t  bReserved;
    uint16_t wPlanes;
    uint16_t wBitCount;
    uint32_t dwBytesInRes;
    uint32_t dwImageOffset;
};

struct ICONDIR {
    uint16_t idReserved;
    uint16_t idType;
    uint16_t idCount;
};

struct ICO_BITMAPINFOHEADER {
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
#pragma pack(pop)

// Monta bloco BMP 32bpp (bottom-up BGRA) + AND mask zerada
std::vector<uint8_t> buildBmpBlock(const uint8_t* rgba, int w, int h)
{
    ICO_BITMAPINFOHEADER hdr{};
    hdr.biSize = sizeof(ICO_BITMAPINFOHEADER);
    hdr.biWidth = w;
    hdr.biHeight = h * 2; // inclui AND mask
    hdr.biPlanes = 1;
    hdr.biBitCount = 32;
    hdr.biCompression = 0;

    // Pixels BGRA, bottom-up
    int rowBytes = w * 4;
    std::vector<uint8_t> pixels(rowBytes * h);
    for (int y = 0; y < h; y++) {
        const uint8_t* src = rgba + (size_t)(h - 1 - y) * w * 4;
        uint8_t* dst = pixels.data() + (size_t)y * rowBytes;
        for (int x = 0; x < w; x++) {
            dst[x * 4 + 0] = src[x * 4 + 2]; // B
            dst[x * 4 + 1] = src[x * 4 + 1]; // G
            dst[x * 4 + 2] = src[x * 4 + 0]; // R
            dst[x * 4 + 3] = src[x * 4 + 3]; // A
        }
    }

    // AND mask: 1 bit por pixel, linhas alinhadas a 4 bytes, tudo zero (opaco via alpha)
    int maskRowBytes = ((w + 31) / 32) * 4;
    std::vector<uint8_t> mask(maskRowBytes * h, 0x00);

    std::vector<uint8_t> block(sizeof(hdr) + pixels.size() + mask.size());
    memcpy(block.data(), &hdr, sizeof(hdr));
    memcpy(block.data() + sizeof(hdr), pixels.data(), pixels.size());
    memcpy(block.data() + sizeof(hdr) + pixels.size(), mask.data(), mask.size());
    return block;
}

// Monta bloco PNG (usado para 256x256 — padrão moderno Windows Vista+)
std::vector<uint8_t> buildPngBlock(const uint8_t* rgba, int w, int h)
{
    std::vector<uint8_t> buf;
    stbi_write_png_to_func(
        [](void* ctx, void* data, int size) {
            auto* v = reinterpret_cast<std::vector<uint8_t>*>(ctx);
            auto* b = reinterpret_cast<uint8_t*>(data);
            v->insert(v->end(), b, b + size);
        },
        &buf, w, h, 4, rgba, w * 4
    );
    return buf;
}

int main(int argc, char* argv[])
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    std::cout << "=== PNG/JPG -> ICO Converter (Max Quality) ===\n\n";

    if (argc < 2) {
        std::cout << "Como usar: arraste uma imagem PNG ou JPG sobre este .exe\n\n";
        std::cout << "Pressione Enter para sair...";
        std::cin.get();
        return 1;
    }

    std::string inputPath = argv[1];
    fs::path p(inputPath);

    if (!fs::exists(p)) {
        std::cerr << "Arquivo nao encontrado: " << inputPath << "\n";
        std::cin.get();
        return 1;
    }

    std::string ext = p.extension().string();
    // Converte extensao para lowercase
    for (auto& c : ext) c = (char)tolower((unsigned char)c);

    if (ext != ".png" && ext != ".jpg" && ext != ".jpeg" && ext != ".bmp" && ext != ".tga") {
        std::cerr << "Formato nao suportado: " << ext << "\n";
        std::cerr << "Use PNG, JPG, BMP ou TGA.\n";
        std::cin.get();
        return 1;
    }

    std::string outputPath = (p.parent_path() / p.stem()).string() + ".ico";

    std::cout << "Entrada : " << inputPath << "\n";
    std::cout << "Saida   : " << outputPath << "\n\n";

    // Carrega imagem original em RGBA
    int srcW = 0, srcH = 0, srcC = 0;
    uint8_t* srcData = stbi_load(inputPath.c_str(), &srcW, &srcH, &srcC, 4);
    if (!srcData) {
        std::cerr << "Erro ao carregar imagem: " << stbi_failure_reason() << "\n";
        std::cin.get();
        return 1;
    }

    std::cout << "Imagem original: " << srcW << "x" << srcH
        << " (" << srcC << " canal(is))\n\n";

    // Todos os tamanhos padrao de ICO (Windows usa todos eles)
    const int sizes[] = { 256, 128, 96, 64, 48, 32, 24, 16 };
    const int numSizes = (int)(sizeof(sizes) / sizeof(sizes[0]));

    std::vector<std::vector<uint8_t>> blocks;
    blocks.reserve(numSizes);

    for (int sz : sizes) {
        std::cout << "  Gerando " << sz << "x" << sz << "px ... ";

        std::vector<uint8_t> resized((size_t)sz * sz * 4);

        // stb_image_resize2: filtro de alta qualidade com suporte a alpha correto
        stbir_resize_uint8_srgb(
            srcData, srcW, srcH, srcW * 4,
            resized.data(), sz, sz, sz * 4,
            STBIR_RGBA
        );

        std::vector<uint8_t> block;
        if (sz == 256) {
            block = buildPngBlock(resized.data(), sz, sz);
            std::cout << "PNG  | " << block.size() << " bytes\n";
        }
        else {
            block = buildBmpBlock(resized.data(), sz, sz);
            std::cout << "BMP  | " << block.size() << " bytes\n";
        }
        blocks.push_back(std::move(block));
    }

    stbi_image_free(srcData);

    // ---- Monta arquivo .ICO ----
    std::ofstream out(outputPath, std::ios::binary);
    if (!out) {
        std::cerr << "\nErro: nao foi possivel criar o arquivo de saida.\n";
        std::cerr << "Verifique permissoes da pasta.\n";
        std::cin.get();
        return 1;
    }

    // ICONDIR header
    ICONDIR dir{};
    dir.idReserved = 0;
    dir.idType = 1; // ICO
    dir.idCount = (uint16_t)numSizes;
    out.write(reinterpret_cast<char*>(&dir), sizeof(dir));

    // Offset inicial dos dados (logo apos header + todas as entries)
    uint32_t dataOffset = (uint32_t)(sizeof(ICONDIR) + sizeof(ICONDIRENTRY) * numSizes);

    // Directory entries
    for (int i = 0; i < numSizes; i++) {
        int sz = sizes[i];
        ICONDIRENTRY entry{};
        entry.bWidth = (sz == 256) ? 0 : (uint8_t)sz; // 0 = 256 no formato ICO
        entry.bHeight = (sz == 256) ? 0 : (uint8_t)sz;
        entry.bColorCount = 0;
        entry.bReserved = 0;
        entry.wPlanes = 1;
        entry.wBitCount = 32;
        entry.dwBytesInRes = (uint32_t)blocks[i].size();
        entry.dwImageOffset = dataOffset;
        out.write(reinterpret_cast<char*>(&entry), sizeof(entry));
        dataOffset += (uint32_t)blocks[i].size();
    }

    // Blocos de imagem
    for (auto& block : blocks) {
        out.write(reinterpret_cast<const char*>(block.data()), block.size());
    }

    out.close();

    std::cout << "\n[OK] ICO gerado com sucesso!\n";
    std::cout << "Tamanhos: 256, 128, 96, 64, 48, 32, 24, 16 px\n";
    std::cout << "Arquivo : " << outputPath << "\n\n";
    std::cout << "Pressione Enter para sair...";
    std::cin.get();
    return 0;
}