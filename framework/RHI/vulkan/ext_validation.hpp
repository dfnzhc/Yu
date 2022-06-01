//
// Created by 秋鱼 on 2022/6/1.
//

#pragma once

#include <vulkan/vulkan_core.h>
#include "instance_properties.hpp"

namespace yu::vk {

void CheckDebugReportInstanceEXT(InstanceProperties& ip, bool gpuValidation);

void SetupDebugMessenger(VkInstance instance);
void DestroyDebugMessenger(VkInstance instance);

} // namespace yu::vk