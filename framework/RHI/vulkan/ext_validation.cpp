//
// Created by 秋鱼 on 2022/6/1.
//

#include "ext_validation.hpp"

#include <logger.hpp>
#include "error.hpp"
#include "initializers.hpp"

#include <vulkan/vulkan.hpp>

namespace yu::vk {

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugReportFlagsEXT flags,
                                                    VkDebugReportObjectTypeEXT /*type*/,
                                                    uint64_t /*object*/,
                                                    size_t /*location*/,
                                                    int32_t /*message_code*/,
                                                    const char* layer_prefix,
                                                    const char* message,
                                                    void* /*user_data*/)
{
    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
        LOG_ERROR("{}: {}", layer_prefix, message);
    } else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
        LOG_WARN("{}: {}", layer_prefix, message);
    } else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) {
        LOG_WARN("{}: {}", layer_prefix, message);
    } else {
        LOG_INFO("{}: {}", layer_prefix, message);
    }

    return VK_FALSE;
}

static VkDebugReportCallbackEXT DebugReporter = nullptr;
static bool CanUseDebugReport = false;
VkValidationFeaturesEXT features;

void CheckDebugReportInstanceEXT(InstanceProperties& ip, bool gpuValidation)
{
    if (!gpuValidation) {
        return;
    }

    const VkValidationFeatureEnableEXT featuresRequested[] =
        {VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
         VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT,
         VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT};

    const char instanceExtensionName[] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
    const char instanceLayerName[] = "VK_LAYER_KHRONOS_validation";

    CanUseDebugReport = ip.addLayer(instanceLayerName) && ip.addExtension(instanceExtensionName);
    if (CanUseDebugReport) {
        features = validationFeatures();
        features.pNext = ip.pNext;
        features.enabledValidationFeatureCount = _countof(featuresRequested);
        features.pEnabledValidationFeatures = featuresRequested;

        ip.pNext = &features;
    }
}

void SetupDebugMessenger(VkInstance instance)
{
    // 不能使用 Debug 扩展，跳过 Debug Messenger 的创建
    if (!CanUseDebugReport) {
        return;
    }

    auto func = (PFN_vkCreateDebugReportCallbackEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
    if (func) {
        VkDebugReportCallbackCreateInfoEXT debugReportCallbackInfo = {
            VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT};
        debugReportCallbackInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT |
            VK_DEBUG_REPORT_WARNING_BIT_EXT |
            VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
        debugReportCallbackInfo.pfnCallback = DebugCallback;
        VK_CHECK(func(instance, &debugReportCallbackInfo, nullptr, &DebugReporter));
    }
}

void DestroyDebugMessenger(VkInstance instance)
{
    // 没有创建 Debug reportor
    if (!DebugReporter) {
        return;
    }
    auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
    if (func) {
        func(instance, DebugReporter, nullptr);
        DebugReporter = nullptr;
    }
}

} // namespace yu::vk
