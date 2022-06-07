//
// Created by 秋鱼 on 2022/5/11.
//

#pragma once

namespace yu::vk {

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

} // namespace yu::vk