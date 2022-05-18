//
// Created by 秋鱼 on 2022/5/11.
//

#include "vk_appbase.hpp"
#include "vk_error.hpp"
#include <vulkan/vulkan_win32.h>

namespace ST {

bool vk_AppBase::prepare(Platform& platform)
{
    if (!Application::prepare(platform)) {
        return false;
    }
    
    initVulkan();
    

    return true;
}

void vk_AppBase::update(float delta_time)
{
    Application::update(delta_time);
}

void vk_AppBase::finish()
{
}

bool vk_AppBase::resize(const uint32_t width, const uint32_t height)
{

    return true;
}

void vk_AppBase::input_event(const InputEvent& input_event)
{
    Application::input_event(input_event);
}

void vk_AppBase::initVulkan()
{
    LOG_INFO("Init vulkan...")
    // first create vulkan instance
    createInstance();

}

void vk_AppBase::createInstance()
{
    LOG_INFO("\tCreate vulkan instance...");
    // 
    VkApplicationInfo appInfo{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = name_.c_str(),
        .pEngineName = "Setaria",
        .apiVersion = vulkan_properties_.apiVersion
    };

    std::vector<const char*> instanceExtensions = {VK_KHR_SURFACE_EXTENSION_NAME};

#if defined(_WIN32)
    instanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#else
    throw std::runtime_error("Only support windows so far.")
#endif

    uint32_t extCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
    if (extCount > 0) {
        std::vector<VkExtensionProperties> extensions(extCount);
        if (VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &extCount, extensions.data()))) {
            for (VkExtensionProperties ext : extensions) {
                supported_instance_extensions_.emplace_back(ext.extensionName);
            }
        }
    }

    if (!enabled_instance_extensions_.empty()) {
        for (const char* ext : enabled_instance_extensions_) {
            if (std::find(supported_instance_extensions_.begin(), supported_instance_extensions_.end(), ext)
                == supported_instance_extensions_.end()) {
                throw std::runtime_error("The extension " + std::string{ext} + " is not support at instance level.");
            }
            instanceExtensions.push_back(ext);
        }
    }

    VkInstanceCreateInfo instanceCreateInfo{};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;
    instanceCreateInfo.pNext = nullptr;

    if (!instanceExtensions.empty()) {
        if (vulkan_settings_.validation) {
            instanceExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
        instanceCreateInfo.enabledExtensionCount = (uint32_t) instanceExtensions.size();
        instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();
    }

    const char* validationLayerName = "VK_LAYER_KHRONOS_validation";
    if (vulkan_settings_.validation) {
        // Check if this layer is available at instance level
        uint32_t instanceLayerCount;
        vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
        std::vector<VkLayerProperties> instanceLayerProperties(instanceLayerCount);
        vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayerProperties.data());
        bool validationLayerPresent = false;
        for (VkLayerProperties layer : instanceLayerProperties) {
            if (strcmp(layer.layerName, validationLayerName) == 0) {
                validationLayerPresent = true;
                break;
            }
        }
        if (validationLayerPresent) {
            instanceCreateInfo.ppEnabledLayerNames = &validationLayerName;
            instanceCreateInfo.enabledLayerCount = 1;
        } else {
            throw std::runtime_error("Validation layer VK_LAYER_KHRONOS_validation not present, validation is disabled");
        }
    }

    VK_CHECK(vkCreateInstance(&instanceCreateInfo, nullptr, &instance_));
}

} // namespace ST