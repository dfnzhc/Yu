//
// Created by 秋鱼 on 2022/5/19.
//

#pragma once

#include <vulkan/vulkan.h>

#define VK_FLAGS_NONE 0
#define DEFAULT_FENCE_TIMEOUT 100000000000

namespace ST::VK {

void SetupDebugMessenger(VkInstance instance,
                         const VkAllocationCallbacks* pAllocator,
                         VkDebugUtilsMessengerEXT* pDebugMessenger);
void DestroyDebugMessenger(VkInstance instance,
                           VkDebugUtilsMessengerEXT pDebugMessenger,
                           const VkAllocationCallbacks* pAllocator);

std::string PhysicalDeviceTypeString(VkPhysicalDeviceType type);

VkBool32 GetSupportedDepthFormat(VkPhysicalDevice physicalDevice, VkFormat* depthFormat);

VkShaderModule LoadShader(const char* fileName, VkDevice device);

} // namespace ST