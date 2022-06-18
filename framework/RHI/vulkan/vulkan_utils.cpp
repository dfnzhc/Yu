//
// Created by 秋鱼 on 2022/6/7.
//

#include <San/utils/utils.hpp>
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

uint32_t SizeOfFormat(VkFormat format)
{
    switch (format) {
        case VK_FORMAT_R8_SINT:
            return 1;//(BYTE)
        case VK_FORMAT_R8_UINT:
            return 1;//(UNSIGNED_BYTE)1
        case VK_FORMAT_R16_SINT:
            return 2;//(SHORT)2
        case VK_FORMAT_R16_UINT:
            return 2;//(UNSIGNED_SHORT)2
        case VK_FORMAT_R32_SINT:
            return 4;//(SIGNED_INT)4
        case VK_FORMAT_R32_UINT:
            return 4;//(UNSIGNED_INT)4
        case VK_FORMAT_R32_SFLOAT:
            return 4;//(FLOAT)

        case VK_FORMAT_R8G8_SINT:
            return 2 * 1;//(BYTE)
        case VK_FORMAT_R8G8_UINT:
            return 2 * 1;//(UNSIGNED_BYTE)1
        case VK_FORMAT_R16G16_SINT:
            return 2 * 2;//(SHORT)2
        case VK_FORMAT_R16G16_UINT:
            return 2 * 2; // (UNSIGNED_SHORT)2
        case VK_FORMAT_R32G32_SINT:
            return 2 * 4;//(SIGNED_INT)4
        case VK_FORMAT_R32G32_UINT:
            return 2 * 4;//(UNSIGNED_INT)4
        case VK_FORMAT_R32G32_SFLOAT:
            return 2 * 4;//(FLOAT)

        case VK_FORMAT_UNDEFINED:
            return 0;//(BYTE) (UNSIGNED_BYTE) (SHORT) (UNSIGNED_SHORT)
        case VK_FORMAT_R32G32B32_SINT:
            return 3 * 4;//(SIGNED_INT)4
        case VK_FORMAT_R32G32B32_UINT:
            return 3 * 4;//(UNSIGNED_INT)4
        case VK_FORMAT_R32G32B32_SFLOAT:
            return 3 * 4;//(FLOAT)

        case VK_FORMAT_R8G8B8A8_SINT:
            return 4 * 1;//(BYTE)
        case VK_FORMAT_R8G8B8A8_UINT:
            return 4 * 1;//(UNSIGNED_BYTE)1
        case VK_FORMAT_R16G16B16A16_SINT:
            return 4 * 2;//(SHORT)2
        case VK_FORMAT_R16G16B16A16_UINT:
            return 4 * 2;//(UNSIGNED_SHORT)2
        case VK_FORMAT_R32G32B32A32_SINT:
            return 4 * 4;//(SIGNED_INT)4
        case VK_FORMAT_R32G32B32A32_UINT:
            return 4 * 4;//(UNSIGNED_INT)4
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            return 4 * 4;//(FLOAT)
    }

    return 0;
}

} // namespace yu::vk