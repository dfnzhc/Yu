//
// Created by 秋鱼 on 2022/6/3.
//

#include "ext_hdr.hpp"
#include <vulkan/vulkan_win32.h>

namespace yu::vk {

PFN_vkGetDeviceProcAddr g_vkGetDeviceProcAddr;

// Functions for regular HDR ex: HDR10
PFN_vkGetPhysicalDeviceSurfaceCapabilities2KHR g_vkGetPhysicalDeviceSurfaceCapabilities2KHR;
PFN_vkGetPhysicalDeviceSurfaceFormats2KHR g_vkGetPhysicalDeviceSurfaceFormats2KHR;
PFN_vkSetHdrMetadataEXT g_vkSetHdrMetadataEXT;

static VkPhysicalDeviceSurfaceInfo2KHR s_PhysicalDeviceSurfaceInfo2KHR;

static VkSurfaceFullScreenExclusiveWin32InfoEXT s_SurfaceFullScreenExclusiveWin32InfoEXT;
static VkSurfaceFullScreenExclusiveInfoEXT s_SurfaceFullScreenExclusiveInfoEXT;

static VkDisplayNativeHdrSurfaceCapabilitiesAMD s_DisplayNativeHdrSurfaceCapabilitiesAMD;

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
