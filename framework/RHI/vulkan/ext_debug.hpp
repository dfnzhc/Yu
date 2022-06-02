//
// Created by 秋鱼 on 2022/6/1.
//

#pragma once

#include "instance_properties.hpp"
namespace yu::vk {

bool CheckDebugUtilsInstanceEXT(InstanceProperties& ip);

void SetupDebugMessenger(VkInstance instance);
void DestroyDebugMessenger(VkInstance instance);

} // namespace yu::vk
