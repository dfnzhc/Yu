//
// Created by 秋鱼 on 2022/6/7.
//

#include <filesystem.hpp>
#include <logger.hpp>
#include "vulkan_utils.hpp"
#include "error.hpp"

namespace yu::vk {

VkShaderStageFlagBits GetShaderType(std::string_view fileName)
{
    auto ext = San::GetFileExtension(fileName);

    if (ext == "vert") {
        return VK_SHADER_STAGE_VERTEX_BIT;
    } else if (ext == "frag") {
        return VK_SHADER_STAGE_FRAGMENT_BIT;
    } else if (ext == "comp") {
        return VK_SHADER_STAGE_COMPUTE_BIT;
    } else if (ext == "geom") {
        return VK_SHADER_STAGE_GEOMETRY_BIT;
    } else if (ext == "tesc") {
        return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    } else if (ext == "tese") {
        return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    } else if (ext == "rgen") {
        return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    } else if (ext == "rahit") {
        return VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
    } else if (ext == "rchit") {
        return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    } else if (ext == "rint") {
        return VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
    } else if (ext == "rmiss") {
        return VK_SHADER_STAGE_MISS_BIT_KHR;
    } else if (ext == "rcall") {
        return VK_SHADER_STAGE_CALLABLE_BIT_KHR;
    }

    throw std::runtime_error("No proper shader type.");
}

VkShaderModule LoadShader(std::string_view fileName, VkDevice device)
{
    std::ifstream is{std::string{fileName}, std::ios::binary | std::ios::in | std::ios::ate};

    if (!is.is_open()) {
        LOG_ERROR("Can not open shader file: {}.", fileName);
        return VK_NULL_HANDLE;
    }

    size_t size = is.tellg();
    if (size == 0) {
        LOG_ERROR("Shader file: {} is empty.", fileName);
        return VK_NULL_HANDLE;
    }

    is.seekg(0, std::ios::beg);
    std::vector<char> shaderCode(size);
    is.read(shaderCode.data(), size);
    is.close();

    VkShaderModule shaderModule;
    VkShaderModuleCreateInfo moduleCreateInfo{};
    moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCreateInfo.codeSize = size;
    moduleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

    VK_CHECK(vkCreateShaderModule(device, &moduleCreateInfo, nullptr, &shaderModule));

    return shaderModule;
}

} // namespace yu::vk