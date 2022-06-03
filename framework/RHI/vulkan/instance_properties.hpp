//
// Created by 秋鱼 on 2022/6/1.
//

#pragma once

#include "properties.hpp"

namespace yu::vk {

/**
 * @brief 用于创建 vulkan 实例的属性，包括启用的扩展和中间层
 */
struct InstanceProperties : public Properties
{
#ifdef NDEUBG
    bool enabled_validation = false;
#else
    bool enabled_validation = true;
#endif

    InstanceProperties();
};

} // yu::vk