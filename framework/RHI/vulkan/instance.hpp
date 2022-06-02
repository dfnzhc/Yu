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
    
private:
    InstanceProperties properties_;

    VkInstance handle_{VK_NULL_HANDLE};
};

} // yu::vk