//
// Created by 秋鱼 on 2022/5/19.
//

#include "vk_utils.hpp"
#include "vk_error.hpp"

#include <vulkan/vulkan.hpp>

namespace ST::VK {

VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsMessengerCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
#if !defined( NDEBUG )
    if (pCallbackData->messageIdNumber == 648835635) {
        // UNASSIGNED-khronos-Validation-debug-build-warning-message
        return VK_FALSE;
    }
    if (pCallbackData->messageIdNumber == 767975156) {
        // UNASSIGNED-BestPractices-vkCreateInstance-specialuse-extension
        return VK_FALSE;
    }
#endif
    std::stringstream debugMessage;

    debugMessage << vk::to_string(static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>( messageSeverity )) << ": "
                 << vk::to_string(static_cast<vk::DebugUtilsMessageTypeFlagsEXT>( messageType )) << ":\n";
    debugMessage << "\t"
                 << "messageIDName   = <" << pCallbackData->pMessageIdName << ">\n";
    debugMessage << "\t"
                 << "messageIdNumber = " << pCallbackData->messageIdNumber << "\n";
    debugMessage << "\t"
                 << "message         = <" << pCallbackData->pMessage << ">\n";
    if (0 < pCallbackData->queueLabelCount) {
        debugMessage << "\t"
                     << "Queue Labels:\n";
        for (uint32_t i = 0; i < pCallbackData->queueLabelCount; i++) {
            debugMessage << "\t\t"
                         << "labelName = <" << pCallbackData->pQueueLabels[i].pLabelName << ">\n";
        }
    }
    if (0 < pCallbackData->cmdBufLabelCount) {
        debugMessage << "\t"
                     << "CommandBuffer Labels:\n";
        for (uint32_t i = 0; i < pCallbackData->cmdBufLabelCount; i++) {
            debugMessage << "\t\t"
                         << "labelName = <" << pCallbackData->pCmdBufLabels[i].pLabelName << ">\n";
        }
    }
    if (0 < pCallbackData->objectCount) {
        debugMessage << "\t"
                     << "Objects:\n";
        for (uint32_t i = 0; i < pCallbackData->objectCount; i++) {
            debugMessage << "\t\t"
                         << "Object " << i << "\n";
            debugMessage << "\t\t\t"
                         << "objectType   = "
                         << vk::to_string(static_cast<vk::ObjectType>( pCallbackData->pObjects[i].objectType )) << "\n";
            debugMessage << "\t\t\t"
                         << "objectHandle = " << pCallbackData->pObjects[i].objectHandle << "\n";
            if (pCallbackData->pObjects[i].pObjectName) {
                debugMessage << "\t\t\t"
                             << "objectName   = <" << pCallbackData->pObjects[i].pObjectName << ">\n";
            }
        }
    }

    LOG_DEBUG(debugMessage.str());

    return VK_TRUE;
}

void SetupDebugMessenger(VkInstance instance,
                         const VkAllocationCallbacks* pAllocator,
                         VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

    if (func == nullptr) {
        throw std::runtime_error("Can not get the function 'vkCreateDebugUtilsMessengerEXT'.");
    }

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugCreateInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugCreateInfo.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    debugCreateInfo.pfnUserCallback = debugUtilsMessengerCallback;

    VK_CHECK(func(instance, &debugCreateInfo, pAllocator, pDebugMessenger));
}

void DestroyDebugMessenger(VkInstance instance,
                           VkDebugUtilsMessengerEXT pDebugMessenger,
                           const VkAllocationCallbacks* pAllocator)
{
    auto func =
        (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, pDebugMessenger, pAllocator);
    }
}

std::string PhysicalDeviceTypeString(VkPhysicalDeviceType type)
{
    switch (type) {
#define STR(r) case VK_PHYSICAL_DEVICE_TYPE_ ##r: return #r
        STR(OTHER);
        STR(INTEGRATED_GPU);
        STR(DISCRETE_GPU);
        STR(VIRTUAL_GPU);
        STR(CPU);
#undef STR
        default:
            return "UNKNOWN_DEVICE_TYPE";
    }
}

VkBool32 GetSupportedDepthFormat(VkPhysicalDevice physicalDevice, VkFormat* depthFormat)
{
    // 由于所有的深度格式都可能是可选的，需要找到一个合适的深度格式来使用
    // 从最高精度的打包格式开始
    std::vector<VkFormat> depthFormats = {
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM
    };

    for (auto& format : depthFormats) {
        VkFormatProperties formatProps;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProps);
        // 格式必须支持深度模板附着，以获得最佳的平铺效果
        if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            *depthFormat = format;
            return true;
        }
    }

    return false;
}

VkShaderModule LoadShader(const char* fileName, VkDevice device)
{
    std::ifstream is{fileName, std::ios::binary | std::ios::in | std::ios::ate};

    if (!is.is_open()) {
        LOG_ERROR("Can not open shader file: {}.", fileName);
        return VK_NULL_HANDLE;
    }

    size_t size = is.tellg();
    if (size == 0) {
        LOG_ERROR("Shader file: {} is empty.", fileName);
        return VK_NULL_HANDLE;
    }

    is.seekg(0, std::ios::beg);
    std::vector<char> shaderCode(size);
    is.read(shaderCode.data(), size);
    is.close();

    VkShaderModule shaderModule;
    VkShaderModuleCreateInfo moduleCreateInfo{};
    moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCreateInfo.codeSize = size;
    moduleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

    VK_CHECK(vkCreateShaderModule(device, &moduleCreateInfo, nullptr, &shaderModule));
    
    return shaderModule;
}

} // namespace ST::VK