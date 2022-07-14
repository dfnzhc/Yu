//
// Created by 秋鱼 on 2022/6/17.
//

#include <common/math_utils.hpp>
#include "static_buffer.hpp"
#include "initializers.hpp"
#include "error.hpp"

namespace yu::vk {

void StaticBuffer::create(const VulkanDevice& device, uint32_t totalSize, bool bUseStaging, std::string_view name)
{
    device_ = &device;
    total_size_ = totalSize;

    use_video_buffer_ = bUseStaging;

    // 在系统上创建缓冲区，绑定并进行映射
    {
#ifdef USE_VMA
        auto bufferInfo = bufferCreateInfo();
        bufferInfo.size = totalSize;
        bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        if (bUseStaging)
            bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        allocInfo.flags = VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
        allocInfo.pUserData = const_cast<char*>(name.data());

        VK_CHECK(vmaCreateBuffer(device.getAllocator(),
                                 &bufferInfo,
                                 &allocInfo,
                                 &buffer_,
                                 &buffer_allocation_,
                                 nullptr));

        VK_CHECK(vmaMapMemory(device.getAllocator(), buffer_allocation_, (void**) &data_));
#else
        VkBufferUsageFlags usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        if (bUseStaging)
            usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        VK_CHECK(device_->createBuffer(usage,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                       total_size_,
                                       &buffer_,
                                       &device_memory_,
                                       false,
                                       (void**) &data_));
#endif
    }

    // 创建显存上的缓冲区，前面创建的缓冲区成为暂存缓冲区
    if (bUseStaging) {
#ifdef USE_VMA
        auto bufferInfo = bufferCreateInfo();
        bufferInfo.size = totalSize;
        bufferInfo.usage =
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        allocInfo.flags = VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
        allocInfo.pUserData = const_cast<char*>(name.data());

        VK_CHECK(vmaCreateBuffer(device.getAllocator(),
                                 &bufferInfo,
                                 &allocInfo,
                                 &video_buffer_,
                                 &video_allocation_,
                                 nullptr));
#else
        VK_CHECK(device_->createBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                       total_size_,
                                       &video_buffer_,
                                       &video_memory_,
                                       false,
                                       nullptr));
#endif
    }
}

void StaticBuffer::destroy()
{
    if (use_video_buffer_) {
#ifdef USE_VMA
        vmaDestroyBuffer(device_->getAllocator(), video_buffer_, video_allocation_);
#else
        vkFreeMemory(device_->getHandle(), video_memory_, nullptr);
        vkDestroyBuffer(device_->getHandle(), video_buffer_, nullptr);
#endif
        video_buffer_ = VK_NULL_HANDLE;
    }

    if (buffer_ != VK_NULL_HANDLE) {
#ifdef USE_VMA
        vmaUnmapMemory(device_->getAllocator(), buffer_allocation_);
        vmaDestroyBuffer(device_->getAllocator(), buffer_, buffer_allocation_);
#else
        vkUnmapMemory(device_->getHandle(), device_memory_);
        vkFreeMemory(device_->getHandle(), device_memory_, nullptr);
        vkDestroyBuffer(device_->getHandle(), buffer_, nullptr);
#endif
        buffer_ = VK_NULL_HANDLE;
    }

}
bool StaticBuffer::allocBuffer(uint32_t numberOfElements,
                               uint32_t elementSizeInByte,
                               void** pData,
                               VkDescriptorBufferInfo* pDesc)
{
    std::lock_guard<std::mutex> lock(mutex_);

    uint32_t size = AlignUp(numberOfElements * elementSizeInByte, 256u);
    assert(memory_offset_ + size < total_size_);

    *pData = (void*) (data_ + memory_offset_);

    pDesc->buffer = use_video_buffer_ ? video_buffer_ : buffer_;
    pDesc->offset = memory_offset_;
    pDesc->range = size;

    memory_offset_ += size;

    return true;
}

bool StaticBuffer::allocBuffer(uint32_t numberOfElements,
                               uint32_t elementSizeInByte,
                               const void* pInitData,
                               VkDescriptorBufferInfo* pDesc)
{
    void* pData;
    if (allocBuffer(numberOfElements, elementSizeInByte, &pData, pDesc)) {
        memcpy(pData, pInitData, numberOfElements * elementSizeInByte);
        return true;
    }

    return false;
}

void StaticBuffer::uploadData(VkCommandBuffer cmdBuf)
{
    if (!use_video_buffer_)
        return;

    VkBufferCopy region;
    region.srcOffset = 0;
    region.dstOffset = 0;
    region.size = total_size_;

    vkCmdCopyBuffer(cmdBuf, buffer_, video_buffer_, 1, &region);
}

void StaticBuffer::freeUploadHeap()
{
    if (!use_video_buffer_ || buffer_ == VK_NULL_HANDLE)
        return;

#ifdef USE_VMA
    vmaUnmapMemory(device_->getAllocator(), buffer_allocation_);
    vmaDestroyBuffer(device_->getAllocator(), buffer_, buffer_allocation_);
#else
    //release upload heap
    vkUnmapMemory(device_->getHandle(), device_memory_);
    vkFreeMemory(device_->getHandle(), device_memory_, nullptr);
    vkDestroyBuffer(device_->getHandle(), buffer_, nullptr);
    device_memory_ = VK_NULL_HANDLE;
#endif
    buffer_ = VK_NULL_HANDLE;
}

} // yu::vk