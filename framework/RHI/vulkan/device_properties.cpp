//
// Created by 秋鱼 on 2022/6/3.
//

#include <logger.hpp>
#include "device_properties.hpp"
#include "error.hpp"

namespace yu::vk {

void DeviceProperties::init(VkPhysicalDevice physicalDevice)
{
    physical_device = physicalDevice;

    uint32_t extensionCount;
    VK_CHECK(vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extensionCount, nullptr));
    extension_properties.resize(extensionCount);
    VK_CHECK(vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extensionCount, extension_properties.data()));

    vkGetPhysicalDeviceFeatures(physical_device, &features);
    vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);
    vkGetPhysicalDeviceProperties(physical_device, &device_properties);

    subgroup_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES;
    subgroup_properties.pNext = nullptr;

    device_properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    device_properties2.pNext = &subgroup_properties;
    vkGetPhysicalDeviceProperties2(physical_device, &device_properties2);

    // query the queue family
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queue_family_count, nullptr);
    assert(queue_family_count > 0);
    queue_family_properties.resize(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queue_family_count, queue_family_properties.data());
}

std::pair<std::string, std::string> DeviceProperties::getDeviceInfo() const
{
    std::string apiVersion = fmt::format("{}.{}.{}",
                                         (device_properties.apiVersion >> 22),
                                         ((device_properties.apiVersion >> 12) & 0x3ff),
                                         (device_properties.apiVersion & 0xfff));

    return {device_properties.deviceName, apiVersion};
}

uint32_t DeviceProperties::getQueueFamilyIndex(VkQueueFlagBits queueFlags) const
{
    // 专门用于计算的队列
    // 尝试找到一个仅支持计算，而不支持图形的队列族索引
    if (queueFlags & VK_QUEUE_COMPUTE_BIT) {
        for (uint32_t i = 0; i < static_cast<uint32_t>(queue_family_properties.size()); i++) {
            if ((queue_family_properties[i].queueFlags & queueFlags)
                && ((queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0)) {
                return i;
            }
        }
    }

    // 用于传输的专用队列
    // 尝试找到一个仅支持传输，而不支持图形和计算的队列家族索引
    if (queueFlags & VK_QUEUE_TRANSFER_BIT) {
        for (uint32_t i = 0; i < static_cast<uint32_t>(queue_family_properties.size()); i++) {
            if ((queue_family_properties[i].queueFlags & queueFlags)
                && ((queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0)
                && ((queue_family_properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0)) {
                return i;
            }
        }
    }

    // 对于其他队列类型，则返回第一个支持所要求的标志的队列
    for (uint32_t i = 0; i < static_cast<uint32_t>(queue_family_properties.size()); i++) {
        if (queue_family_properties[i].queueFlags & queueFlags) {
            return i;
        }
    }

    throw std::runtime_error("Could not find a matching queue family index");
}

} // namespace yu::vk