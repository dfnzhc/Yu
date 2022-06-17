//
// Created by 秋鱼 on 2022/6/12.
//

#include "dynamic_buffer.hpp"
#include "error.hpp"
#include <San/utils/math_utils.hpp>
#include <common/math_utils.hpp>
#include "initializers.hpp"

namespace yu::vk {

void DynamicBuffer::create(const VulkanDevice& device, uint32_t numberOfFrames, uint32_t totalSize, std::string_view name)
{
    device_ = &device;

    total_size_ = AlignUp(totalSize, 256u);

    mem_.create(numberOfFrames, total_size_);

    // 创建一个可用于处理 uniform、索引、顶点数据的缓冲区
#ifdef USE_VMA
    auto bufferInfo = bufferCreateInfo(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                       total_size_);

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    allocInfo.flags = VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
    allocInfo.pUserData = const_cast<char*>(name.data());

    auto allocator = const_cast<VmaAllocator>(device_->getAllocator());
    VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer_, &buffer_allocation_, nullptr));

    VK_CHECK(vmaMapMemory(allocator, buffer_allocation_, (void**) &data_));
#else
    VkBufferCreateInfo buf_info = {};
    buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buf_info.pNext = nullptr;
    buf_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    buf_info.size = total_size_;
    buf_info.queueFamilyIndexCount = 0;
    buf_info.pQueueFamilyIndices = nullptr;
    buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buf_info.flags = 0;
    VK_CHECK(vkCreateBuffer(device_->getHandle(), &buf_info, nullptr, &buffer_));

    VkMemoryRequirements mem_reqs;
    vkGetBufferMemoryRequirements(device_->getHandle(), buffer_, &mem_reqs);

    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.pNext = nullptr;
    alloc_info.memoryTypeIndex = 0;
    alloc_info.memoryTypeIndex = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    alloc_info.allocationSize = mem_reqs.size;
    alloc_info.memoryTypeIndex = 0;

    bool pass = device_->getProperties().getMemoryType(mem_reqs.memoryTypeBits,
                                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                                       &alloc_info.memoryTypeIndex);
    assert(pass && "No mappable, coherent memory");

    VK_CHECK(vkAllocateMemory(device_->getHandle(), &alloc_info, nullptr, &device_memory_));
    VK_CHECK(vkMapMemory(device_->getHandle(), device_memory_, 0, mem_reqs.size, 0, (void**) &data_));
    VK_CHECK(vkBindBufferMemory(device_->getHandle(), buffer_, device_memory_, 0));
#endif
}

void DynamicBuffer::destroy()
{
#ifdef USE_VMA
    auto allocator = const_cast<VmaAllocator>(device_->getAllocator());
    vmaUnmapMemory(allocator, buffer_allocation_);
    vmaDestroyBuffer(allocator, buffer_, buffer_allocation_);
    
    buffer_ = VK_NULL_HANDLE;
    buffer_allocation_ = VK_NULL_HANDLE;
#else
    vkUnmapMemory(device_->getHandle(), device_memory_);
    vkFreeMemory(device_->getHandle(), device_memory_, nullptr);
    vkDestroyBuffer(device_->getHandle(), buffer_, nullptr);
    
    buffer_ = VK_NULL_HANDLE;
    device_memory_ = VK_NULL_HANDLE;
#endif

    mem_.destroy();
}

bool DynamicBuffer::allocConstantBuffer(uint32_t size, void** data, VkDescriptorBufferInfo& descOut)
{
    size = AlignUp(size, 256u);

    // 获取分配内存的起始偏移
    uint32_t offset;
    if (!mem_.alloc(size, &offset)) {
        LOG_FATAL("Out of memory of 'Dynamic Buffer', please increase the buffer size.");
        return false;
    }

    // 让 data 指向分配内存的起始位置
    *data = static_cast<void*>(data_ + offset);

    descOut.buffer = buffer_;
    descOut.offset = offset;
    descOut.range = size;

    return true;
}

VkDescriptorBufferInfo DynamicBuffer::allocConstantBuffer(uint32_t size, void* data)
{
    void* temp = nullptr;
    VkDescriptorBufferInfo descBufferInfo{};
    if (allocConstantBuffer(size, &temp, descBufferInfo)) {
        std::memcpy(temp, data, size);
    }

    return descBufferInfo;
}

void DynamicBuffer::setDescriptorSet(int bindIndex, uint32_t size, VkDescriptorSet descriptorSet)
{
    VkDescriptorBufferInfo descBufferInfo{};
    descBufferInfo.buffer = buffer_;
    descBufferInfo.offset = 0;
    descBufferInfo.range = size;

    auto writeDesc = writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, bindIndex, &descBufferInfo);

    vkUpdateDescriptorSets(device_->getHandle(), 1, &writeDesc, 0, nullptr);
}

void DynamicBuffer::beginFrame()
{
    mem_.beginFrame();
}
} // yu::vk
