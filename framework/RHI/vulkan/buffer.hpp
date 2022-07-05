//
// Created by 秋鱼 on 2022/6/29.
//

#pragma once

#include <vulkan/vulkan.h>

namespace yu::vk {

// 与 vulkan 缓冲区有关的构件的抽象
// VkBufferCreateInfo bufferCreate = makeBufferCreateInfo (size, VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT);
// VkBuffer buffer                 = createBuffer(device, bufferCreate);
// VkBufferView bufferView         = 
//                    createBufferView(device, makeBufferViewCreateInfo(buffer, VK_FORMAT_R8G8B8A8_UNORM, size));

/**
 * @brief 缓冲区的创建信息
 * @param usage 指示缓冲区的用途
 * @param flags 一些额外的信息
 */
VkBufferCreateInfo makeBufferCreateInfo(VkDeviceSize size,
                                        VkBufferUsageFlags usage,
                                        VkBufferCreateFlags flags = 0);

/**
 * @brief 缓冲区视图的创建信息
 * @param format 缓冲区的格式
 * @param range 所指定的缓冲区范围
 * @param offset 视图所指定内存在缓冲区的偏移
 */
VkBufferViewCreateInfo makeBufferViewCreateInfo(VkBuffer buffer,
                                                VkFormat format,
                                                VkDeviceSize range,
                                                VkDeviceSize offset = 0,
                                                VkBufferViewCreateFlags flags = 0);

/**
 * @brief 通过描述符的缓冲区信息来设置缓冲区视图创建信息
 */
VkBufferViewCreateInfo makeBufferViewCreateInfo(const VkDescriptorBufferInfo& descrInfo,
                                                VkFormat fmt,
                                                VkBufferViewCreateFlags flags = 0);

/**
 * @brief 创建 vulkan 缓冲区
 */
VkBuffer createBuffer(VkDevice device, VkBufferCreateInfo info);

/**
 * @brief 创建 vulkan 缓冲区的视图
 */
VkBufferView createBufferView(VkDevice device, VkBufferViewCreateInfo info);

/**
* @brief 对备份设备内存的缓冲区的访问进行封装
*/
struct VulkanBuffer
{
    VkDevice device{};
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkDescriptorBufferInfo descriptor{};
    VkDeviceSize size = 0;
    VkDeviceSize alignment = 0;
    void* mapped = nullptr;

    /** @brief 在创建缓冲区的时候设置其用途 */
    VkBufferUsageFlags usageFlags{};
    /** @brief 在创建缓冲区的时候设置它的属性标志 */
    VkMemoryPropertyFlags memoryPropertyFlags{};

    VkResult map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
    void unmap();

    VkResult bind(VkDeviceSize offset = 0);
    void setupDescriptor(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
    void copyTo(void* data, VkDeviceSize size);
    VkResult flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
    VkResult invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

    void destroy();
};

} // yu::vk