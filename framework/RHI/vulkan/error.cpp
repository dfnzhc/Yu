//
// Created by 秋鱼 on 2022/6/1.
//

#include <logger.hpp>
#include "error.hpp"

namespace yu::vk {

const char* GetResultString(VkResult result)
{
    const char* resultString = "unknown";

#define STR(a)                                                                                                         \
  case a:                                                                                                              \
    resultString = #a;                                                                                                 \
    break;

    switch (result) {
        STR(VK_SUCCESS);
        STR(VK_NOT_READY);
        STR(VK_TIMEOUT);
        STR(VK_EVENT_SET);
        STR(VK_EVENT_RESET);
        STR(VK_INCOMPLETE);
        STR(VK_ERROR_OUT_OF_HOST_MEMORY);
        STR(VK_ERROR_OUT_OF_DEVICE_MEMORY);
        STR(VK_ERROR_INITIALIZATION_FAILED);
        STR(VK_ERROR_DEVICE_LOST);
        STR(VK_ERROR_MEMORY_MAP_FAILED);
        STR(VK_ERROR_LAYER_NOT_PRESENT);
        STR(VK_ERROR_EXTENSION_NOT_PRESENT);
        STR(VK_ERROR_FEATURE_NOT_PRESENT);
        STR(VK_ERROR_INCOMPATIBLE_DRIVER);
        STR(VK_ERROR_TOO_MANY_OBJECTS);
        STR(VK_ERROR_FORMAT_NOT_SUPPORTED);
        STR(VK_ERROR_FRAGMENTED_POOL);
        STR(VK_ERROR_OUT_OF_POOL_MEMORY);
        STR(VK_ERROR_INVALID_EXTERNAL_HANDLE);
        STR(VK_ERROR_SURFACE_LOST_KHR);
        STR(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR);
        STR(VK_SUBOPTIMAL_KHR);
        STR(VK_ERROR_OUT_OF_DATE_KHR);
        STR(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR);
        STR(VK_ERROR_VALIDATION_FAILED_EXT);
        STR(VK_ERROR_INVALID_SHADER_NV);
        STR(VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT);
        STR(VK_ERROR_FRAGMENTATION_EXT);
        STR(VK_ERROR_NOT_PERMITTED_EXT);
        STR(VK_ERROR_INVALID_DEVICE_ADDRESS_EXT);
        STR(VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT);
    }
#undef STR

    return resultString;
}

bool CheckResult(VkResult result, const char* msg)
{
    if (result == VK_SUCCESS) {
        return false;
    }
    
    if (result < VK_SUCCESS) {
        if (msg) {
            LOG_FATAL("VkResult: {}({}) - {}", GetResultString(result), result, msg)
        }
        else {
            LOG_FATAL("VkResult: {}({})", GetResultString(result), result)
        }
    }
    
    return false;
}


bool vk::CheckResult(VkResult result, const char* file, int32_t line)
{
    if (result == VK_SUCCESS) {
        return false;
    }
    
    if (result < VK_SUCCESS) {
        LOG_FATAL("{}({}): Vulkan Error: {}", file, line, GetResultString(result))
    }
    
    return false;
}

} // yu::vk