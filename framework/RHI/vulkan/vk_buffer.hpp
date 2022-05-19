//
// Created by 秋鱼 on 2022/5/19.
//

#pragma once

#include <vulkan/vulkan.h>

namespace ST::VK {

/**
* @brief 对备份设备内存的缓冲区的访问进行封装
*/
struct Buffer
{
    VkDevice device;
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkDescriptorBufferInfo descriptor;
    VkDeviceSize size = 0;
    VkDeviceSize alignment = 0;
    void* mapped = nullptr;

    /** @brief 在创建缓冲区的时候设置其用途 */
    VkBufferUsageFlags usageFlags;
    /** @brief 在创建缓冲区的时候设置它的属性标志 */
    VkMemoryPropertyFlags memoryPropertyFlags;

    VkResult map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
    void unmap();

    VkResult bind(VkDeviceSize offset = 0);
    void setupDescriptor(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
    void copyTo(void* data, VkDeviceSize size);
    VkResult flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
    VkResult invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

    void destroy();
};

} // namespace ST::VK
