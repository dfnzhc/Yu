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
        auto buf_info = bufferCreateInfo();
        buf_info.pNext = nullptr;
        buf_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        if (bUseStaging)
            buf_info.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        buf_info.size = totalSize;
        buf_info.queueFamilyIndexCount = 0;
        buf_info.pQueueFamilyIndices = nullptr;
        buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        buf_info.flags = 0;
        VK_CHECK(vkCreateBuffer(device.getHandle(), &buf_info, nullptr, &buffer_));

        // 在系统上分配内存
        VkMemoryRequirements mem_reqs;
        vkGetBufferMemoryRequirements(device.getHandle(), buffer_, &mem_reqs);

        auto alloc_info = memoryAllocateInfo();
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.pNext = nullptr;
        alloc_info.memoryTypeIndex = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        alloc_info.allocationSize = mem_reqs.size;

        bool pass = device_->getProperties().getMemoryType(mem_reqs.memoryTypeBits,
                                                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                                                               | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                                           &alloc_info.memoryTypeIndex);
        assert(pass && "No mappable, coherent memory");
        VK_CHECK(vkAllocateMemory(device.getHandle(), &alloc_info, nullptr, &device_memory_));

        // 绑定缓冲区
        VK_CHECK(vkBindBufferMemory(device.getHandle(), buffer_, device_memory_, 0));

        // 映射该缓冲区，保持映射的状态
        VK_CHECK(vkMapMemory(device.getHandle(), device_memory_, 0, mem_reqs.size, 0, (void**) &data_));
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
        auto buf_info = bufferCreateInfo();
        buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buf_info.pNext = nullptr;
        buf_info.usage =
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        buf_info.size = totalSize;
        buf_info.queueFamilyIndexCount = 0;
        buf_info.pQueueFamilyIndices = nullptr;
        buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        buf_info.flags = 0;
        VK_CHECK(vkCreateBuffer(device.getHandle(), &buf_info, nullptr, &video_buffer_));

        // allocate the buffer in VIDEO memory
        VkMemoryRequirements mem_reqs;
        vkGetBufferMemoryRequirements(device.getHandle(), video_buffer_, &mem_reqs);

        VkMemoryAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.pNext = nullptr;
        alloc_info.memoryTypeIndex = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        alloc_info.allocationSize = mem_reqs.size;

        bool pass = device_->getProperties().getMemoryType(mem_reqs.memoryTypeBits,
                                                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                                           &alloc_info.memoryTypeIndex);
        assert(pass && "No mappable, coherent memory");

        VK_CHECK(vkAllocateMemory(device.getHandle(), &alloc_info, nullptr, &video_memory_));

        // bind buffer
        VK_CHECK(vkBindBufferMemory(device.getHandle(), video_buffer_, video_memory_, 0));
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