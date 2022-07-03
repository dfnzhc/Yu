//
// Created by 秋鱼 on 2022/6/1.
//

#include <logger.hpp>
#include "ext_debug.hpp"
#include "error.hpp"
#include "initializers.hpp"

#include <vulkan/vulkan.hpp>

namespace yu::vk {

static bool CanUseDebugEXT = false;
static VkDebugUtilsMessengerEXT DebugMessenger = nullptr;

VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
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

    debugMessage << ::vk::to_string(static_cast<::vk::DebugUtilsMessageSeverityFlagBitsEXT>( messageSeverity )) << ": "
                 << ::vk::to_string(static_cast<::vk::DebugUtilsMessageTypeFlagsEXT>( messageType )) << ":\n";
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
                         << ::vk::to_string(static_cast<::vk::ObjectType>( pCallbackData->pObjects[i].objectType )) << "\n";
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

bool CheckDebugUtilsInstanceEXT(InstanceProperties& ip)
{
    static constexpr char instanceLayerName[] = "VK_LAYER_KHRONOS_validation";

    CanUseDebugEXT = ip.addLayer(instanceLayerName) && ip.addExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    return CanUseDebugEXT;
}

void SetupDebugMessenger(VkInstance instance)
{
    if (!CanUseDebugEXT) {
        return;
    }

    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (!func) {
        LOG_ERROR("Can not get the function 'vkCreateDebugUtilsMessengerEXT', SetupDebugMessenger() failed.");
    }

    auto debugCreateInfo = debugUtilsMessengerCreateInfoEXT();
    debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugCreateInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugCreateInfo.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    debugCreateInfo.pfnUserCallback = debugUtilsMessengerCallback;

    VK_CHECK(func(instance, &debugCreateInfo, nullptr, &DebugMessenger));
}

void DestroyDebugMessenger(VkInstance instance)
{
    if (!DebugMessenger) {
        return;
    }

    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (!func) {
        LOG_ERROR("Can not get the function 'vkDestroyDebugUtilsMessengerEXT', DestroyDebugMessenger() failed.");
    }

    func(instance, DebugMessenger, nullptr);
    DebugMessenger = nullptr;
}

DebugUtil::DebugUtil(VkDevice device) : device_{device}
{
}

void DebugUtil::setObjectName(const uint64_t object, const std::string& name, VkObjectType t)
{
    if (CanUseDebugEXT) {
        VkDebugUtilsObjectNameInfoEXT s{VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT, nullptr, t, object, name.c_str()};
        vkSetDebugUtilsObjectNameEXT(device_, &s);
    }
}

void DebugUtil::beginLabel(VkCommandBuffer cmdBuf, const std::string& label)
{
    if (CanUseDebugEXT) {
        VkDebugUtilsLabelEXT s{VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT, nullptr, label.c_str(), {1.0f, 1.0f, 1.0f, 1.0f}};
        vkCmdBeginDebugUtilsLabelEXT(cmdBuf, &s);
    }
}

void DebugUtil::endLabel(VkCommandBuffer cmdBuf)
{
    if (CanUseDebugEXT) {
        vkCmdEndDebugUtilsLabelEXT(cmdBuf);
    }
}

void DebugUtil::insertLabel(VkCommandBuffer cmdBuf, const std::string& label)
{
    if (CanUseDebugEXT) {
        VkDebugUtilsLabelEXT s{VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT, nullptr, label.c_str(), {1.0f, 1.0f, 1.0f, 1.0f}};
        vkCmdInsertDebugUtilsLabelEXT(cmdBuf, &s);
    }
}

} // namespace yu::vk
