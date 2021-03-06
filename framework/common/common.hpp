//
// Created by 秋鱼 on 2022/5/11.
//

#pragma once

namespace yu {

constexpr inline std::string AssetPath()
{
    return std::string{YU_ROOT_PATH} + "/data";
}

constexpr inline std::string ShaderFilePath()
{
    return AssetPath() + "/shaders";
}

inline std::string GetSpvShaderFile(const std::string& filename)
{
    return ShaderFilePath() + "/spv/" + filename + ".spv";
}

constexpr inline std::string TextureFilePath()
{
    return AssetPath() + "/textures";
}

inline std::string GetTextureFile(const std::string& filename)
{
    return TextureFilePath() + "/" + filename;
}

constexpr inline std::string ModelFilePath()
{
    return AssetPath() + "/models";
}

inline std::string GetModelFile(const std::string& filename)
{
    return ModelFilePath() + "/" + filename;
}

} // namespace yu