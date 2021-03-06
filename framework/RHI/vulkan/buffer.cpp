//
// Created by 秋鱼 on 2022/7/5.
//

#include "buffer.hpp"

namespace yu::vk {

/**
 * @brief 缓冲区的创建信息
 * @param usage 指示缓冲区的用途
 * @param flags 一些额外的信息
 */
VkBufferCreateInfo makeBufferCreateInfo(VkDeviceSize size,
                                        VkBufferUsageFlags usage,
                                        VkBufferCreateFlags flags)
{
    VkBufferCreateInfo createInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    createInfo.size = size;
    createInfo.usage = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    createInfo.flags = flags;

    return createInfo;
}

/**
 * @brief 缓冲区视图的创建信息
 * @param format 缓冲区的格式
 * @param range 所指定的缓冲区范围
 * @param offset 视图所指定内存在缓冲区的偏移
 */
VkBufferViewCreateInfo makeBufferViewCreateInfo(VkBuffer buffer,
                                                VkFormat format,
                                                VkDeviceSize range,
                                                VkDeviceSize offset,
                                                VkBufferViewCreateFlags flags)
{
    VkBufferViewCreateInfo createInfo = {VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO};
    createInfo.buffer = buffer;
    createInfo.offset = offset;
    createInfo.range = range;
    createInfo.flags = flags;
    createInfo.format = format;

    return createInfo;
}

/**
 * @brief 通过描述符的缓冲区信息来设置缓冲区视图创建信息
 */
VkBufferViewCreateInfo makeBufferViewCreateInfo(const VkDescriptorBufferInfo& descrInfo,
                                                VkFormat fmt,
                                                VkBufferViewCreateFlags flags)
{
    VkBufferViewCreateInfo createInfo = {VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO};
    createInfo.buffer = descrInfo.buffer;
    createInfo.offset = descrInfo.offset;
    createInfo.range = descrInfo.range;
    createInfo.flags = flags;
    createInfo.format = fmt;

    return createInfo;
}

/**
 * @brief 创建 vulkan 缓冲区
 */
VkBuffer createBuffer(VkDevice device, VkBufferCreateInfo info)
{
    VkBuffer buffer;
    VkResult result = vkCreateBuffer(device, &info, nullptr, &buffer);
    assert(result == VK_SUCCESS);
    return buffer;
}

/**
 * @brief 创建 vulkan 缓冲区的视图
 */
VkBufferView createBufferView(VkDevice device, VkBufferViewCreateInfo info)
{
    VkBufferView bufferView;
    VkResult result = vkCreateBufferView(device, &info, nullptr, &bufferView);
    assert(result == VK_SUCCESS);
    return bufferView;
}

/** 
* 映射这个缓冲区的某一个内存范围
* 
* @param size (可选）要映射的内存范围的大小。通过VK_WHOLE_SIZE来映射整个缓冲区范围
* @param offset (可选)从头开始的字节偏移
* 
* @return VkResult of the buffer mapping call
*/
VkResult VulkanBuffer::map(VkDeviceSize size, VkDeviceSize offset)
{
    return vkMapMemory(device, memory, offset, size, 0, &mapped);
}
/**
* 解除对当前内存的映射
*
* @note 不返回结果，因为vkUnmapMemory不会失败
*/
void VulkanBuffer::unmap()
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
VkResult VulkanBuffer::bind(VkDeviceSize offset)
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
void VulkanBuffer::setupDescriptor(VkDeviceSize size, VkDeviceSize offset)
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
void VulkanBuffer::copyTo(void* data, VkDeviceSize size)
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
VkResult VulkanBuffer::flush(VkDeviceSize size, VkDeviceSize offset)
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
VkResult VulkanBuffer::invalidate(VkDeviceSize size, VkDeviceSize offset)
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
void VulkanBuffer::destroy()
{
    if (buffer) {
        vkDestroyBuffer(device, buffer, nullptr);
    }
    if (memory) {
        vkFreeMemory(device, memory, nullptr);
    }
}

} // namespace yu::vk