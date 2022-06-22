//
// Created by 秋鱼 on 2022/6/3.
//

#pragma once

#include <vulkan/vulkan.h>

namespace yu::vk {

VkShaderStageFlagBits GetShaderType(std::string_view fileName);
VkShaderModule LoadShader(std::string_view fileName, VkDevice device);
uint32_t SizeOfFormat(VkFormat format);
uint32_t BitSizeOfFormat(VkFormat format);

template<typename T>
requires std::same_as<decltype(T::sType), VkStructureType>
bool EntityNotSet(T t)
{
    if (t.sType == VK_STRUCTURE_TYPE_APPLICATION_INFO) {
        return true;
    }
    
    return false;
}

} // namespace yu::vk