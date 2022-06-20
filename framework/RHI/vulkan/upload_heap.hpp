//
// Created by 秋鱼 on 2022/6/19.
//

#pragma once
#include <async.hpp>
#include "device.hpp"

namespace yu::vk {

class UploadHeap
{
public:
    void create(const VulkanDevice& device, uint64_t totalSize);
    void destory();

    uint8_t* alloc(uint64_t size, uint64_t align);
    uint8_t* beginAlloc(uint64_t size, uint64_t align);
    void endAlloc();

    void addImageCopy(VkImage image, VkBufferImageCopy region);
    void addImagePreBarrier(VkImageMemoryBarrier imageMemBarrier);
    void addImagePostBarrier(VkImageMemoryBarrier imageMemBarrier);

    void flush();
    void flushAndFinish(bool bDoBarriers = false);

    uint8_t* basePtr() const { return data_begin; }
    VkBuffer getBuffer() const { return buffer_; }
    VkCommandBuffer getCommandBuffer() const { return command_buffer_; }

private:
    const VulkanDevice* device_ = nullptr;
    VkCommandPool command_pool_{};
    VkCommandBuffer command_buffer_{};

    VkBuffer buffer_{};
    VkDeviceMemory device_memory_{};

    VkFence fence_{};

    uint8_t* data_begin = nullptr;    // starting position of upload heap
    uint8_t* data_curr = nullptr;     // current position of upload heap
    uint8_t* data_end = nullptr;      // ending position of upload heap 

    // 用于同步的计数器
    San::Sync allocating_, flushing_;

    std::mutex mutex_{};

    struct IMG_COPY
    {
        VkImage image;
        VkBufferImageCopy region;
    };
    std::vector<IMG_COPY> image_copies_;

    std::vector<VkImageMemoryBarrier> pre_barriers_;
    std::vector<VkImageMemoryBarrier> post_barriers_;
};

} // yu::vk