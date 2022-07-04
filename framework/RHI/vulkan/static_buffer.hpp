//
// Created by 秋鱼 on 2022/6/17.
//

#pragma once

#include "device.hpp"

namespace yu::vk {

class StaticBuffer
{
public:
    void create(const VulkanDevice& device, uint32_t totalSize, bool bUseStaging, std::string_view name);
    void destroy();

    // 分配足够大小的缓冲区，让 pData 指向缓冲区的起始，并设置对应的描述符缓冲区信息
    bool allocBuffer(uint32_t numberOfElements,
                     uint32_t elementSizeInByte,
                     void** pData,
                     VkDescriptorBufferInfo* pDesc);

    // 分配足够大小的缓冲区，用 pInitData 中的数据进行初始化，并设置对应的描述符缓冲区信息
    bool allocBuffer(uint32_t numberOfElements,
                     uint32_t elementSizeInByte,
                     const void* pInitData,
                     VkDescriptorBufferInfo* pDesc);

    // 如果使用了设备上的暂存缓冲区，这个方法会将上传堆(upload heap)中的数据转移到设备上
    void uploadData(VkCommandBuffer cmdBuf);

    // 如果使用了设备上的暂存缓冲区，那么释放上传堆(upload heap)
    void freeUploadHeap();

private:
    const VulkanDevice* device_ = nullptr;

    char* data_ = nullptr;
    uint32_t memory_offset_ = 0;
    uint32_t total_size_ = 0;

    VkBuffer buffer_{};
    VkBuffer video_buffer_{};

    std::mutex mutex_{};
    bool use_video_buffer_ = true;

#ifdef USE_VMA
    VmaAllocation buffer_allocation_{};
    VmaAllocation video_allocation_{};
#else
    VkDeviceMemory device_memory_{};
    VkDeviceMemory video_memory_{};
#endif
};

} // yu::vk