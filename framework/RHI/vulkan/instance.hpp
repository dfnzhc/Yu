//
// Created by 秋鱼 on 2022/6/1.
//

#pragma once

#include "instance_properties.hpp"
#include <window.hpp>

namespace yu::vk {

class VulkanInstance
{
public:
    VulkanInstance(std::string_view appName, InstanceProperties instanceProps);
    ~VulkanInstance();
    
    void createSurface(const San::Window* window);
    void destroySurface();
    
    VkSurfaceKHR getSurface() const { return surface_; }
    VkInstance getHandle() const { return instance_; }
    VkPhysicalDevice getBestDevice() const;
private:
    
    void setEssentialExtensions();
    
    InstanceProperties properties_;
    VkInstance instance_{};
    VkSurfaceKHR surface_{};
};

} // yu::vk