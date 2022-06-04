//
// Created by 秋鱼 on 2022/6/3.
//

#pragma once

#include "properties.hpp"

namespace yu::vk {

struct DeviceProperties : public Properties
{
    void init(VkPhysicalDevice physicalDevice);

    std::pair<std::string, std::string> getDeviceInfo() const;

    VkPhysicalDevice physical_device{};
    
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceMemoryProperties memory_properties{};
    VkPhysicalDeviceProperties device_properties{};
    VkPhysicalDeviceProperties2 device_properties2{};
    VkPhysicalDeviceSubgroupProperties subgroup_properties{};
    
    uint32_t queue_family_count = 0;
    std::vector<VkQueueFamilyProperties> queue_family_properties;

    bool using_fp16 = false;
    bool support_rt10 = false;
    bool support_rt11 = false;
};

} // namespace yu::vk
