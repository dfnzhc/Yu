//
// Created by 秋鱼 on 2022/6/3.
//

#pragma once

#include "instance_properties.hpp"
#include "device_properties.hpp"

namespace yu::vk {

void CheckHDRInstanceEXT(InstanceProperties& ip);
void CheckHDRDeviceEXT(DeviceProperties& dp);

bool AreHDRExtensionsPresent();


} // namespace yu::vk