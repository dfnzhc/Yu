//
// Created by 秋鱼 on 2022/6/3.
//

#include "ext_hdr.hpp"
#include <vulkan/vulkan_win32.h>

namespace yu::vk {

static bool CanUseHdrInstanceEXT = false;
static bool CanUseHdrDeviceEXT = false;

void CheckHDRInstanceEXT(InstanceProperties& ip)
{
    CanUseHdrInstanceEXT = ip.addExtension(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
}

void CheckHDRDeviceEXT(DeviceProperties& dp)
{
    CanUseHdrDeviceEXT = dp.addExtension(VK_EXT_HDR_METADATA_EXTENSION_NAME);
}

} // namespace yu::vk
