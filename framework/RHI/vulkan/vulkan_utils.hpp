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

class VulkanDevice;
// 创建相关的工具
void CreateImageSampler(VkDevice device, float maxAnisotropy, VkSampler& sampler);
void CreateImageView(VkDevice device,
                     VkImage image,
                     VkFormat format,
                     VkImageAspectFlags aspectFlags,
                     VkImageView& imageView);
void CreateImage(const VulkanDevice& device,
                 uint32_t width,
                 uint32_t height,
                 VkFormat format,
                 VkImageTiling tiling,
                 VkImageUsageFlags usage,
                 VkMemoryPropertyFlags properties,
                 VkImage& image,
                 VkDeviceMemory& imageMemory);

VkRenderPass CreateRenderPassOptimal(VkDevice device,
                                     const std::vector<VkAttachmentDescription>& pColorAttachments,
                                     VkAttachmentDescription* pDepthAttachment);

VkFormat GetDepthFormat(const VulkanDevice& device, const std::vector<VkFormat>& formats, VkImageTiling tiling,
                        VkFormatFeatureFlags features);

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

} // namespace yu::vk