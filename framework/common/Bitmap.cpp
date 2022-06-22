//
// Created by 秋鱼 on 2022/6/21.
//

#include "Bitmap.h"
#include "common.hpp"
#include "math_utils.hpp"

#include <stb_image_write.h>
#include <stb_image.h>
#include <stb_image_resize.h>

#include <logger.hpp>
#include <utils.hpp>

using glm::ivec2;
using glm::vec3;

namespace yu {

void Bitmap::initGetSetFuncs()
{
    switch (bitmap_format) {
        case BitmapFormat::UnsignedByte:
            setPixelFunc = &Bitmap::setPixelUnsignedByte;
            getPixelFunc = &Bitmap::getPixelUnsignedByte;
            break;
        case BitmapFormat::Float:
            setPixelFunc = &Bitmap::setPixelFloat;
            getPixelFunc = &Bitmap::getPixelFloat;
            break;
    }
}

void Bitmap::initPixelData(bool bGenMipMap, const void* pData)
{
    uint32_t totalSize = width * height * depth * comp * GetBytesPerComponent(bitmap_format);
    pixels.resize(totalSize);

    if (bGenMipMap) {
        totalSize = 0;
        uint32_t w = width, h = height;
        for (uint32_t i = 1; i < mip_level; i++) {
            uint32_t mipSize = w * h * comp * GetBytesPerComponent(bitmap_format);
            w >>= 1;
            h >>= 1;
            totalSize += mipSize;
        }
        pixels.resize(totalSize);
    } else {
        mip_level = 1;
    }

    if (pData) {
        std::memcpy(pixels.data(), pData, pixels.size());
    }
}

void Bitmap::setPixelFloat(uint32_t x, uint32_t y, const glm::vec4& c)
{
    const uint32_t ofs = comp * (y * width + x);
    auto* data = reinterpret_cast<float*>(pixels.data());
    if (comp > 0) data[ofs + 0] = c.x;
    if (comp > 1) data[ofs + 1] = c.y;
    if (comp > 2) data[ofs + 2] = c.z;
    if (comp > 3) data[ofs + 3] = c.w;
}

glm::vec4 Bitmap::getPixelFloat(uint32_t x, uint32_t y) const
{
    const uint32_t ofs = comp * (y * width + x);
    const auto* data = reinterpret_cast<const float*>(pixels.data());
    return {
        comp > 0 ? data[ofs + 0] : 0.0f,
        comp > 1 ? data[ofs + 1] : 0.0f,
        comp > 2 ? data[ofs + 2] : 0.0f,
        comp > 3 ? data[ofs + 3] : 0.0f};
}

void Bitmap::setPixelUnsignedByte(uint32_t x, uint32_t y, const glm::vec4& c)
{
    const uint32_t ofs = comp * (y * width + x);
    if (comp > 0) pixels[ofs + 0] = uint8_t(c.x * 255.0f);
    if (comp > 1) pixels[ofs + 1] = uint8_t(c.y * 255.0f);
    if (comp > 2) pixels[ofs + 2] = uint8_t(c.z * 255.0f);
    if (comp > 3) pixels[ofs + 3] = uint8_t(c.w * 255.0f);
}

glm::vec4 Bitmap::getPixelUnsignedByte(uint32_t x, uint32_t y) const
{
    const uint32_t ofs = comp * (y * width + x);
    auto r = comp > 0 ? float(pixels[ofs + 0]) / 255.0f : 0.0f;
    auto g = comp > 0 ? float(pixels[ofs + 1]) / 255.0f : 0.0f;
    auto b = comp > 0 ? float(pixels[ofs + 2]) / 255.0f : 0.0f;
    auto a = comp > 0 ? float(pixels[ofs + 3]) / 255.0f : 0.0f;

    return {r, g, b, a};
}

void Bitmap::SaveHDR(const std::string& filename, bool bSaveMip)
{
    LOG_INFO("Writing a {} x {} HDR file to {}", width, height, filename);

    auto* img = pixels.data();

    int ret;

    if (depth == 1) {
        ret = stbi_write_hdr(GetTextureFile(filename.c_str()).c_str(),
                             width,
                             height,
                             comp,
                             reinterpret_cast<float*>(img));
        if (ret == 0) {
            LOG_ERROR("Bitmap::SaveHDR(): Could not save HDR file {}.", filename);
        }
    } 
    else if (depth == 6) {
        auto name = San::GetFileName(filename);
        auto offset = width * height * comp * GetBytesPerComponent(bitmap_format);
        for (uint32_t i = 0; i < depth; i++) {
            std::string layerFile = std::string{name} + "_cubeMap_" + std::to_string(i) + ".hdr";

            ret = stbi_write_hdr(GetTextureFile(layerFile).c_str(),
                                 width,
                                 height,
                                 comp,
                                 reinterpret_cast<float*>(img));
            if (ret == 0) {
                LOG_ERROR("Bitmap::SaveHDR(): Could not save layered HDR file {} {}.", layerFile, i);
            }
            
            img += offset;
        }
    }

    if (bSaveMip && mip_level > 1) {
        auto name = San::GetFileName(filename);
        uint32_t w = width, h = height;
        img = pixels.data();
        for (uint32_t i = 1; i < mip_level; i++) {
            std::string mipFile = std::string{name} + "_mip_" + std::to_string(i) + ".hdr";
            img += w * h * comp * GetBytesPerComponent(bitmap_format);

            w >>= 1;
            h >>= 1;

            ret = stbi_write_hdr(GetTextureFile(mipFile).c_str(), w, h, comp, reinterpret_cast<float*>(img));
            if (ret == 0) {
                LOG_ERROR("Bitmap::SaveHDR(): Could not save HDR file {}.", filename);
            }
        }
    }
}

void Bitmap::SavePNG(const std::string& filename, bool bSaveMip)
{
    LOG_INFO("Writing a {} x {} PNG file to {}", width, height, filename);

    uint8_t* img = pixels.data();

    int ret = stbi_write_png(GetTextureFile(filename).c_str(), width, height, comp, img, 0);
    if (ret == 0) {
        LOG_ERROR("Bitmap::SavePNG(): Could not save PNG file {}.", filename);
    }

    if (bSaveMip && mip_level > 1) {
        auto name = San::GetFileName(filename);
        uint32_t w = width, h = height;
        img = pixels.data();
        for (uint32_t i = 1; i < mip_level; i++) {
            std::string mipFile = std::string{name} + "_mip_" + std::to_string(i) + ".png";
            img += w * h * comp * GetBytesPerComponent(bitmap_format);

            w >>= 1;
            h >>= 1;

            ret = stbi_write_png(GetTextureFile(mipFile).c_str(), w, h, comp, img, 0);
            if (ret == 0) {
                LOG_ERROR("Bitmap::SavePNG(): Could not save PNG file {}.", mipFile);
            }
        }
    }
}

int GetMipMapLevels(int w, int h)
{
    int levels = 1;
    while ((w | h) >> levels)
        levels += 1;
    return levels;
}

// 只有当图片大小是 2 的幂次，且图片的长宽相等时，才生成 mip map
bool CanTextureGenMipMap(uint32_t w, uint32_t h)
{
    return w == h && IsPowerOfTwo(w) && IsPowerOfTwo(h);
}

Bitmap LoadTextureFormFile(std::string_view filename, bool bGenMipMap)
{
    int texWidth, texHeight, texComp;
    stbi_uc* pixels =
        stbi_load(GetTextureFile(filename.data()).c_str(), &texWidth, &texHeight, &texComp, STBI_rgb_alpha);

    if (!pixels) {
        LOG_ERROR("Failed to load [{}] texture", filename);
        return {};
    }

    uint32_t imageSize = texWidth * texHeight * texComp;
    std::vector<uint8_t> data(imageSize);

    uint32_t mipLevels = GetMipMapLevels(texWidth, texHeight);

    uint32_t w = texWidth, h = texHeight;

    bGenMipMap = bGenMipMap && CanTextureGenMipMap(w, h);
    if (bGenMipMap) {
//        uint32_t mipMapSize = (imageSize * 3) >> 1;
        uint32_t newSize = 0;
        for (uint32_t i = 1; i < mipLevels; i++) {
            uint32_t mipSize = w * h * texComp;
            w >>= 1;
            h >>= 1;
            newSize += mipSize;
        }
        data.resize(imageSize);
    }

    auto* dst = data.data();
    auto* src = dst;
    std::memcpy(dst, pixels, imageSize);
    stbi_image_free(pixels);

    if (bGenMipMap) {
        w = texWidth, h = texHeight;
        for (uint32_t i = 1; i < mipLevels; i++) {
            dst += w * h * texComp;

            stbir_resize_uint8(src, w, h, 0, dst, w / 2, h / 2, 0, texComp);

            w >>= 1;
            h >>= 1;
            src = dst;
        }
    }

    Bitmap ret{static_cast<uint32_t>(texWidth),
               static_cast<uint32_t>(texHeight),
               static_cast<uint32_t>(texComp),
               BitmapFormat::UnsignedByte, data.data(),
               mipLevels,
               bGenMipMap};
    ret.name = San::GetFileName(filename);

    return ret;
}

void Float24to32(uint32_t w, uint32_t h, const float* img24, float* img32)
{
    const uint32_t numPixels = w * h;
    for (uint32_t i = 0; i != numPixels; i++) {
        *img32++ = *img24++;
        *img32++ = *img24++;
        *img32++ = *img24++;
        *img32++ = 1.0f;
    }
}

Bitmap LoadHDRTextureFormFile(std::string_view fileName, bool bIsCubemap, bool bGenMipMap)
{
    int texWidth, texHeight;
    float* img = nullptr;

    auto cubeMapName = std::string{San::GetFileName(fileName)} + "_cross.hdr";
    bool loadCrossFile = false;
    if (bIsCubemap && San::IsFileExist(GetTextureFile(cubeMapName))) {
        img = stbi_loadf(GetTextureFile(cubeMapName).c_str(), &texWidth, &texHeight, nullptr, 3);
        loadCrossFile = true;
    } else {
        img = stbi_loadf(GetTextureFile(fileName.data()).c_str(), &texWidth, &texHeight, nullptr, 3);
    }

    if (!img) {
        LOG_ERROR("Failed to load [{}] texture", fileName);
        return {};
    }

    const uint8_t texComp = 4;

    uint32_t imageSize = texWidth * texHeight * texComp;
    uint32_t mipLevels = GetMipMapLevels(texWidth, texHeight);

    uint32_t w = texWidth, h = texHeight;
    bGenMipMap = CanTextureGenMipMap(w, h);
    if (bGenMipMap) {
        imageSize = 0;
        for (uint32_t i = 1; i < mipLevels; i++) {
            uint32_t mipSize = w * h * texComp;
            w >>= 1;
            h >>= 1;
            imageSize += mipSize;
        }
    }

    std::vector<float> data(imageSize);

    auto* dst = data.data();
    auto* src = dst;

    w = texWidth, h = texHeight;
    Float24to32(w, h, img, dst);
    stbi_image_free((void*) img);

    if (bGenMipMap) {
        for (uint32_t i = 1; i < mipLevels; i++) {
            dst += w * h * texComp;

            stbir_resize_float_generic(src, w, h, 0,
                                       dst, w / 2, h / 2, 0,
                                       texComp, STBIR_ALPHA_CHANNEL_NONE, 0,
                                       STBIR_EDGE_CLAMP,
                                       STBIR_FILTER_CUBICBSPLINE,
                                       STBIR_COLORSPACE_LINEAR,
                                       nullptr);

            w >>= 1;
            h >>= 1;
            src = dst;
        }
    }

    Bitmap ret{static_cast<uint32_t>(texWidth),
               static_cast<uint32_t>(texHeight),
               static_cast<uint32_t>(texComp),
               BitmapFormat::Float, data.data(),
               mipLevels,
               bGenMipMap};

    ret.is_cubeMap = true;
    ret.name = San::GetFileName(fileName);

    if (bIsCubemap && !loadCrossFile) {
        Bitmap cross = ConvertEquirectangularMapToVerticalCross(ret);

        cross.SaveHDR(cubeMapName);
        return cross;
    }

    return ret;
}

// 把图片坐标转换成立体盒子上的三维坐标
vec3 texCoordsToXYZ(int i, int j, int faceID, int faceSize)
{
    const float Pi = 2.0f * float(i) / faceSize;
    const float Pj = 2.0f * float(j) / faceSize;

    if (faceID == 0) return {Pi - 1.0f, -1.0f, 1.0f - Pj};
    if (faceID == 1) return {1.0f, Pi - 1.0f, 1.0f - Pj};
    if (faceID == 2) return {1.0f - Pi, 1.0f, 1.0f - Pj};
    if (faceID == 3) return {-1.0f, 1.0 - Pi, 1.0 - Pj};
    if (faceID == 4) return {Pj - 1.0f, Pi - 1.0f, 1.0f};
    if (faceID == 5) return {1.0f - Pj, Pi - 1.0f, -1.0f};

    return {};
}

Bitmap ConvertEquirectangularMapToVerticalCross(const Bitmap& b)
{
    if (!b.is_cubeMap) {
        LOG_WARN("Can not convert {} to vertical cross, cause it is not a cube map", b.name);
        return {};
    }

    const int faceSize = b.width / 4;

    // 转换后十字图片的大小
    const int w = faceSize * 4;
    const int h = faceSize * 3;

    Bitmap ret{static_cast<uint32_t>(w), static_cast<uint32_t>(h), b.comp, b.bitmap_format};
    ret.name = b.name + "_cross";
    ret.is_cubeMap = true;

    const ivec2 kFaceOffsets[] =
        {
            ivec2(0, faceSize),
            ivec2(faceSize, faceSize),
            ivec2(2 * faceSize, faceSize),
            ivec2(3 * faceSize, faceSize),
            ivec2(faceSize, 0),
            ivec2(faceSize, 2 * faceSize)
        };

    const int clampW = static_cast<int>(b.width) - 1;
    const int clampH = static_cast<int>(b.height) - 1;

    for (int face = 0; face < 6; ++face) {
        for (int i = 0; i < faceSize; ++i) {
            for (int j = 0; j < faceSize; ++j) {
                const vec3 P = texCoordsToXYZ(i, j, face, faceSize);
                const float R = hypot(P.x, P.y);
                const float theta = atan2(P.y, P.x);
                const float phi = atan2(P.z, R);
                // float point source coordinates
                const auto Uf = float(2.0f * faceSize * (theta + PI_F) / PI_F);
                const auto Vf = float(2.0f * faceSize * (PI_F / 2.0f - phi) / PI_F);
                // 4-samples for bilinear interpolation
                const auto U1 = glm::clamp(int(floor(Uf)), 0, clampW);
                const auto V1 = glm::clamp(int(floor(Vf)), 0, clampH);
                const auto U2 = glm::clamp(U1 + 1, 0, clampW);
                const auto V2 = glm::clamp(V1 + 1, 0, clampH);
                // fractional part
                const auto s = static_cast<float>(Uf) - static_cast<float>(U1);
                const auto t = static_cast<float>(Vf) - static_cast<float>(V1);
                // fetch 4-samples
                const vec4 A = b.getPixel(U1, V1);
                const vec4 B = b.getPixel(U2, V1);
                const vec4 C = b.getPixel(U1, V2);
                const vec4 D = b.getPixel(U2, V2);
                // bilinear interpolation
                const vec4 color = A * (1 - s) * (1 - t) + B * (s) * (1 - t) + C * (1 - s) * t + D
                    * (s) * (t);
                ret.setPixel(static_cast<uint32_t>(i + kFaceOffsets[face].x),
                             static_cast<uint32_t>(j + kFaceOffsets[face].y),
                             color);
            }
        }
    }

    return ret;
}

Bitmap ConvertVerticalCrossToCubeMapFaces(const Bitmap& b)
{
    const int faceWidth = b.width / 4;
    const int faceHeight = b.height / 3;

    Bitmap cubemap(faceWidth, faceHeight, 6, b.comp, b.bitmap_format);
    cubemap.name = b.name + "_cubeMap";
    cubemap.is_cubeMap = true;

    const uint8_t* src = b.pixels.data();
    uint8_t* dst = cubemap.pixels.data();

    /*
          ------
          | +Y |
     ---------------------
     | -X | -Z | +X | +Z |
     ---------------------
          | -Y |
          ------
    */
    const int pixelSize = cubemap.comp * GetBytesPerComponent(cubemap.bitmap_format);
    for (int face = 0; face != 6; ++face) {
        for (int j = 0; j != faceHeight; ++j) {
            for (int i = 0; i != faceWidth; ++i) {
                int x = 0;
                int y = 0;

                switch (face) {
                    // GL_TEXTURE_CUBE_MAP_POSITIVE_X
                    case 0:
                        x = i;
                        y = faceHeight + j;
                        break;

                        // GL_TEXTURE_CUBE_MAP_NEGATIVE_X
                    case 1:
                        x = 2 * faceWidth + i;
                        y = 1 * faceHeight + j;
                        break;

                        // GL_TEXTURE_CUBE_MAP_POSITIVE_Y
                    case 2:
                        x = 2 * faceWidth - (i + 1);
                        y = 1 * faceHeight - (j + 1);
                        break;

                        // GL_TEXTURE_CUBE_MAP_NEGATIVE_Y
                    case 3:
                        x = 2 * faceWidth - (i + 1);
                        y = 3 * faceHeight - (j + 1);
                        break;

                        // GL_TEXTURE_CUBE_MAP_POSITIVE_Z
                    case 4:
                        x = 3 * faceWidth + i;
                        y = 1 * faceHeight + j;
                        break;

                        // GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
                    case 5:
                        x = faceWidth + i;
                        y = faceHeight + j;
                        break;
                }

                memcpy(dst, src + (y * b.width + x) * pixelSize, pixelSize);

                dst += pixelSize;
            }
        }
    }

    return cubemap;
}

} // yu