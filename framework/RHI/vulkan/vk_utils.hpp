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

VkShaderStageFlagBits GetShaderType(std::string_view fileName);
VkShaderModule LoadShader(std::string_view fileName, VkDevice device);

void SetImageLayout(
    VkCommandBuffer cmdbuffer,
    VkImage image,
    VkImageLayout oldImageLayout,
    VkImageLayout newImageLayout,
    VkImageSubresourceRange subresourceRange,
    VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

// Uses a fixed sub resource layout with first mip level and layer
void SetImageLayout(
    VkCommandBuffer cmdbuffer,
    VkImage image,
    VkImageAspectFlags aspectMask,
    VkImageLayout oldImageLayout,
    VkImageLayout newImageLayout,
    VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

/** @brief Insert an image memory barrier into the command buffer */
void InsertImageMemoryBarrier(
    VkCommandBuffer cmdbuffer,
    VkImage image,
    VkAccessFlags srcAccessMask,
    VkAccessFlags dstAccessMask,
    VkImageLayout oldImageLayout,
    VkImageLayout newImageLayout,
    VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask,
    VkImageSubresourceRange subresourceRange);

} // namespace ST