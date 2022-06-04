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
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);
    assert(queue_family_count > 0);
    queue_family_properties.resize(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_family_properties.data());
}

std::pair<std::string, std::string> DeviceProperties::getDeviceInfo() const
{
    std::string apiVersion = fmt::format("{}.{}.{}",
                                         (device_properties.apiVersion >> 22),
                                         ((device_properties.apiVersion >> 12) & 0x3ff),
                                         (device_properties.apiVersion & 0xfff));

    return {device_properties.deviceName, apiVersion};
}

} // namespace yu::vk