//
// Created by 秋鱼 on 2022/6/3.
//

#include <logger.hpp>
#include "ext_float.hpp"

namespace yu::vk {

static VkPhysicalDeviceFloat16Int8FeaturesKHR FP16Features = {};
static VkPhysicalDevice16BitStorageFeatures Storage16BitFeatures = {};

void CheckFP16DeviceEXT(DeviceProperties& dp)
{
    bool bFp16Enabled = dp.addExtension(VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME);

    if (bFp16Enabled) {
        // Query 16 bit storage
        Storage16BitFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES;
        VkPhysicalDeviceFeatures2 features = {};
        features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        features.pNext = &Storage16BitFeatures;
        vkGetPhysicalDeviceFeatures2(dp.physical_device, &features);

        // Query 16 bit ops
        FP16Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT16_INT8_FEATURES_KHR;
        features.pNext = &FP16Features;
        vkGetPhysicalDeviceFeatures2(dp.physical_device, &features);

        bFp16Enabled = Storage16BitFeatures.storageBuffer16BitAccess && FP16Features.shaderFloat16;
    }

    if (bFp16Enabled) {
        // Query 16 bit storage
        Storage16BitFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES;
        Storage16BitFeatures.pNext = dp.pNext;

        // Query 16 bit ops
        FP16Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT16_INT8_FEATURES_KHR;
        FP16Features.pNext = &Storage16BitFeatures;

        dp.pNext = &FP16Features;
    }

    dp.using_fp16 = bFp16Enabled;
}

} // yu::vk
