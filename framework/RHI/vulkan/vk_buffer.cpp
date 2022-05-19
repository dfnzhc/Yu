//
// Created by 秋鱼 on 2022/5/19.
//

#include "vk_buffer.hpp"
namespace ST::VK {

/** 
* 映射这个缓冲区的某一个内存范围
* 
* @param size (可选）要映射的内存范围的大小。通过VK_WHOLE_SIZE来映射整个缓冲区范围
* @param offset (可选)从头开始的字节偏移
* 
* @return VkResult of the buffer mapping call
*/
VkResult Buffer::map(VkDeviceSize size, VkDeviceSize offset)
{
    return vkMapMemory(device, memory, offset, size, 0, &mapped);
}
/**
* 解除对当前内存的映射
*
* @note 不返回结果，因为vkUnmapMemory不会失败
*/
void Buffer::unmap()
{
    if (mapped) {
        vkUnmapMemory(device, memory);
        mapped = nullptr;
    }
}

/** 
* 将分配的内存块附加到缓冲区上
* 
* @param offset (Optional) 要绑定的内存区域的字节偏移量（从开始）
* 
* @return VkResult of the bindBufferMemory call
*/
VkResult Buffer::bind(VkDeviceSize offset)
{
    return vkBindBufferMemory(device, buffer, memory, offset);
}

/**
* 设置当前缓冲区的默认描述符
*
* @param size (可选) 描述符的内存大小
* @param offset (可选) 从头开始的字节偏移量
*
*/
void Buffer::setupDescriptor(VkDeviceSize size, VkDeviceSize offset)
{
    descriptor.offset = offset;
    descriptor.buffer = buffer;
    descriptor.range = size;
}

/**
* 复制指定的数据到映射的缓冲区
* 
* @param data 指向要复制的数据的指针
* @param size 要复制的数据的大小，以机器单位计算
*
*/
void Buffer::copyTo(void* data, VkDeviceSize size)
{
    assert(mapped);
    memcpy(mapped, data, size);
}

/** 
* 刷新缓冲区的内存范围，使其对设备可见
*
* @note 只对 non-coherent memory 有要求
*
* @param size (Optional) 要刷新的内存大小。通过VK_WHOLE_SIZE来刷新整个缓冲区范围
* @param offset (Optional) 从开始的字节偏移量
*
* @return VkResult of the flush call
*/
VkResult Buffer::flush(VkDeviceSize size, VkDeviceSize offset)
{
    VkMappedMemoryRange mappedRange = {};
    mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mappedRange.memory = memory;
    mappedRange.offset = offset;
    mappedRange.size = size;
    return vkFlushMappedMemoryRanges(device, 1, &mappedRange);
}

/**
* Invalidate 缓冲区的一个内存范围，使其对 host 可见
*
* @note 只对 non-coherent memory 有要求
*
* @param size (Optional) 要刷新的内存大小。通过VK_WHOLE_SIZE来刷新整个缓冲区范围
* @param offset (Optional) 从开始的字节偏移量
*
* @return VkResult of the invalidate call
*/
VkResult Buffer::invalidate(VkDeviceSize size, VkDeviceSize offset)
{
    VkMappedMemoryRange mappedRange = {};
    mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mappedRange.memory = memory;
    mappedRange.offset = offset;
    mappedRange.size = size;
    return vkInvalidateMappedMemoryRanges(device, 1, &mappedRange);
}

/** 
* 释放该缓冲区持有的所有Vulkan资源
*/
void Buffer::destroy()
{
    if (buffer) {
        vkDestroyBuffer(device, buffer, nullptr);
    }
    if (memory) {
        vkFreeMemory(device, memory, nullptr);
    }
}
} // namespace ST::VK
