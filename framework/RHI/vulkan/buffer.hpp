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
inline VkBufferCreateInfo makeBufferCreateInfo(VkDeviceSize size,
                                               VkBufferUsageFlags usage,
                                               VkBufferCreateFlags flags = 0)
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
inline VkBufferViewCreateInfo makeBufferViewCreateInfo(VkBuffer buffer,
                                                       VkFormat format,
                                                       VkDeviceSize range,
                                                       VkDeviceSize offset = 0,
                                                       VkBufferViewCreateFlags flags = 0)
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
inline VkBufferViewCreateInfo makeBufferViewCreateInfo(const VkDescriptorBufferInfo& descrInfo,
                                                       VkFormat fmt,
                                                       VkBufferViewCreateFlags flags = 0)
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
inline VkBuffer createBuffer(VkDevice device, VkBufferCreateInfo info)
{
    VkBuffer buffer;
    VkResult result = vkCreateBuffer(device, &info, nullptr, &buffer);
    assert(result == VK_SUCCESS);
    return buffer;
}

/**
 * @brief 创建 vulkan 缓冲区的视图
 */
inline VkBufferView createBufferView(VkDevice device, VkBufferViewCreateInfo info)
{
    VkBufferView bufferView;
    VkResult result = vkCreateBufferView(device, &info, nullptr, &bufferView);
    assert(result == VK_SUCCESS);
    return bufferView;
}

inline VkDeviceAddress getBufferDeviceAddressKHR(VkDevice device, VkBuffer buffer)
{
    VkBufferDeviceAddressInfo info = {VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO_KHR};
    info.buffer = buffer;
    return vkGetBufferDeviceAddressKHR(device, &info);
}
inline VkDeviceAddress getBufferDeviceAddress(VkDevice device, VkBuffer buffer)
{
    VkBufferDeviceAddressInfo info = {VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
    info.buffer = buffer;
    return vkGetBufferDeviceAddress(device, &info);
}

} // yu::vk