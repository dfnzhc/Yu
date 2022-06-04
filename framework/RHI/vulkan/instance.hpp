//
// Created by 秋鱼 on 2022/6/1.
//

#pragma once

#include "instance_properties.hpp"

namespace yu::vk {

class Instance
{
public:
    Instance(std::string_view appName, InstanceProperties instanceProps);
    
    ~Instance();
    
    VkInstance getHandle() const { return instance_; }
    
    VkPhysicalDevice getBestDevice();
private:
    
    void setEssentialExtensions();
    
    InstanceProperties properties_;

    VkInstance instance_{VK_NULL_HANDLE};
};

} // yu::vk