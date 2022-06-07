//
// Created by 秋鱼 on 2022/6/3.
//

#pragma once

#include <vulkan/vulkan.h>

namespace yu::vk {

VkShaderStageFlagBits GetShaderType(std::string_view fileName);
VkShaderModule LoadShader(std::string_view fileName, VkDevice device);

} // namespace yu::vk