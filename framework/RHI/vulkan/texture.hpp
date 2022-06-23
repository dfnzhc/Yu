//
// Created by 秋鱼 on 2022/6/19.
//

#pragma once

#include <common/Bitmap.h>
#include "device.hpp"

#include "common/stb_inc.hpp"
#include "upload_heap.hpp"

//#undef USE_VMA

namespace yu::vk {

class Texture
{
public:
    Texture() = default;
    ~Texture() = default;

    void create(const VulkanDevice& device, VkImageCreateInfo& createInfo, std::string_view name);
    void createRenderTarget(const VulkanDevice& device,
                            uint32_t width,
                            uint32_t height,
                            VkFormat format,
                            VkSampleCountFlagBits sampleBits,
                            VkImageUsageFlagBits usageBits,
                            VkImageCreateFlagBits createBits,
                            std::string_view name);
    void createDepthStencil(const VulkanDevice& device,
                            uint32_t width,
                            uint32_t height,
                            VkFormat format,
                            VkSampleCountFlagBits sampleBits,
                            std::string_view name);

    void createFromFile2D(const VulkanDevice& device,
                          UploadHeap& uploadHeap,
                          std::string_view fileName,
                          VkImageUsageFlags flags = 0);

    void destory();

    void createRTV(VkImageView* pImageView, int mipLevel, VkFormat format);
    void createSRV(VkImageView* pImageView, int mipLevel = -1);
    void createDSV(VkImageView* pImageView);
    void createCubeSRV(VkImageView* pImageView);

    uint32_t getWidth() const { return info_.width; }
    uint32_t getHeight() const { return info_.height; }
    VkFormat getFormat() const { return format_; }
    VkImage getImage() const { return image_; }

private:
    void createVulkanImage(std::string_view name, VkImageUsageFlags flags = 0);
    void upload(UploadHeap& uploadHeap);
    
private:
    const VulkanDevice* device_ = nullptr;

    ImageInfo info_{};
    VkFormat format_{};
    VkImage image_{};
    
    Bitmap bitmap_{};

#ifdef USE_VMA
    VmaAllocation image_allocation_{};
#else
    VkDeviceMemory device_memory_{};
#endif

};

} // yu::vk