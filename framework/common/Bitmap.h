//
// Created by 秋鱼 on 2022/6/21.
//

#pragma once

#include <glm/glm.hpp>

using glm::vec4;

namespace yu {

enum class BitmapFormat
{
    UnsignedByte, Float,
};

inline uint32_t GetBytesPerComponent(BitmapFormat fmt)
{
    if (fmt == BitmapFormat::UnsignedByte) return 1;
    if (fmt == BitmapFormat::Float) return 4;
    return 0;
}

struct Bitmap
{
    Bitmap() = default;

    Bitmap(uint32_t w, uint32_t h, uint32_t comp, BitmapFormat fmt, uint32_t mipLevel = 0, bool bGenMipMap = false)
        : width(w), height(h), comp(comp), bitmap_format(fmt), mip_level(mipLevel)
    {
        initGetSetFuncs();
        initPixelData(bGenMipMap);
    }

    Bitmap(uint32_t w,
           uint32_t h,
           uint32_t d,
           uint32_t comp,
           BitmapFormat fmt,
           uint32_t mipLevel = 0,
           bool bGenMipMap = false)
        : width(w), height(h), depth(d), comp(comp), bitmap_format(fmt), mip_level(mipLevel)
    {
        initGetSetFuncs();
        initPixelData(bGenMipMap);
    }

    Bitmap(uint32_t w,
           uint32_t h,
           uint32_t comp,
           BitmapFormat fmt,
           const void* pData,
           uint32_t mipLevel = 0,
           bool bGenMipMap = false)
        : width(w), height(h), comp(comp), bitmap_format(fmt), mip_level(mipLevel)
    {
        initGetSetFuncs();
        initPixelData(bGenMipMap, pData);
    }

    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t depth = 1;
    uint32_t comp = 3;
    uint32_t mip_level = 1;
    BitmapFormat bitmap_format = BitmapFormat::UnsignedByte;
    bool is_cubeMap = false;
    std::string name{};

    using Pixels = std::vector<uint8_t>;
    Pixels pixels;

    void setPixel(uint32_t x, uint32_t y, const glm::vec4& c)
    {
        (*this.*setPixelFunc)(x, y, c);
    }

    glm::vec4 getPixel(uint32_t x, uint32_t y) const
    {
        return ((*this.*getPixelFunc)(x, y));
    }

    void SaveHDR(const std::string& filename, bool bSaveMip = false);
    void SavePNG(const std::string& filename, bool bSaveMip = false);

private:
    using setPixel_t = void (Bitmap::*)(uint32_t, uint32_t, const glm::vec4&);
    using getPixel_t = glm::vec4(Bitmap::*)(uint32_t, uint32_t) const;
    setPixel_t setPixelFunc = &Bitmap::setPixelUnsignedByte;
    getPixel_t getPixelFunc = &Bitmap::getPixelUnsignedByte;

    void initGetSetFuncs();

    void initPixelData(bool bGenMipMap = false, const void* pData = nullptr);

    void setPixelFloat(uint32_t x, uint32_t y, const glm::vec4& c);
    glm::vec4 getPixelFloat(uint32_t x, uint32_t y) const;

    void setPixelUnsignedByte(uint32_t x, uint32_t y, const glm::vec4& c);
    glm::vec4 getPixelUnsignedByte(uint32_t x, uint32_t y) const;
};

Bitmap LoadTextureFormFile(std::string_view filename, bool bGenMipMap = false);

Bitmap LoadHDRTextureFormFile(std::string_view fileName, bool bIsCubemap, bool bGenMipMap = false);

Bitmap ConvertEquirectangularMapToVerticalCross(const Bitmap& b);
Bitmap ConvertVerticalCrossToCubeMapFaces(const Bitmap& b);

} // yu