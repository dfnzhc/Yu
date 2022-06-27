//
// Created by 秋鱼 on 2022/6/12.
//

#pragma once

#include <common/buffer_ring.hpp>
#include "device.hpp"
namespace yu::vk {

/**
 * @brief 一个动态缓冲区的抽象，通过从一块巨大的内存中分配内存，使用环形缓冲区来实现
 */
class DynamicBuffer
{
public:
    void create(const VulkanDevice& device, uint32_t numberOfFrames, uint32_t totalSize, std::string_view name);
    void destroy();

    bool allocConstantBuffer(uint32_t size, void** data, VkDescriptorBufferInfo& descOut);
    VkDescriptorBufferInfo allocConstantBuffer(uint32_t size, void* data);

    void setDescriptorSet(int bindIndex, uint32_t size, VkDescriptorSet descriptorSet);

    void beginFrame();

private:
    const VulkanDevice* device_ = nullptr;

    BufferRing mem_;
    uint32_t total_size_{};
    char* data_ = nullptr;

    VkBuffer buffer_{};

#ifdef USE_VMA
    VmaAllocation buffer_allocation_ = VK_NULL_HANDLE;
#else
    VkDeviceMemory device_memory_ = VK_NULL_HANDLE;
#endif
};

} // yu::vk