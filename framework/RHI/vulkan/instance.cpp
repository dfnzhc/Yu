//
// Created by 秋鱼 on 2022/6/1.
//

#include <logger.hpp>
#include "instance.hpp"
#include "initializers.hpp"
#include "error.hpp"
#include "ext_debug.hpp"
#include "ext_hdr.hpp"

#include <vulkan/vulkan_win32.h>

namespace yu::vk {

Instance::Instance(std::string_view appName, InstanceProperties instanceProps)
    : properties_{std::move(instanceProps)}
{
    auto appInfo = applicationInfo();
    appInfo.pNext = nullptr;
    appInfo.pApplicationName = appName.data();
    appInfo.applicationVersion = 1;
    appInfo.pEngineName = "YU Engine";
    appInfo.engineVersion = 1;
    appInfo.apiVersion = VK_API_VERSION_1_3;

    setEssentialExtensions();

    auto instCI = instanceCreateInfo();
    instCI.pNext = nullptr;
    instCI.pApplicationInfo = &appInfo;
    instCI.enabledLayerCount = static_cast<uint32_t>(properties_.enabled_layers.size());
    instCI.ppEnabledLayerNames = properties_.enabled_layers.data();
    instCI.enabledExtensionCount = static_cast<uint32_t>(properties_.enabled_extensions.size());
    instCI.ppEnabledExtensionNames = properties_.enabled_extensions.data();

    VK_CHECK(vkCreateInstance(&instCI, nullptr, &instance_));

    // create debug messenger
    SetupDebugMessenger(instance_);
}

Instance::~Instance()
{
    // destroy debug messenger
    DestroyDebugMessenger(instance_);

    if (instance_ != VK_NULL_HANDLE) {
        vkDestroyInstance(instance_, nullptr);
        instance_ = VK_NULL_HANDLE;
    }
}

void Instance::setEssentialExtensions()
{
    // set the extensions and layers
    if (properties_.enabled_validation) {
        CheckDebugUtilsInstanceEXT(properties_);
    }
    CheckHDRInstanceEXT(properties_);
    
    properties_.addExtension(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
    properties_.addExtension(VK_KHR_SURFACE_EXTENSION_NAME);
}

uint32_t GetScore(VkPhysicalDevice physicalDevice)
{
    uint32_t score = 0;

    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
    // 选择使用 feature 可以评估更多的设备信息，从而选择更优的设备
    // VkPhysicalDeviceFeatures deviceFeatures;
    // vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);
    switch (deviceProperties.deviceType) {
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            score += 1000;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            score += 10000;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            score += 100;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            score += 10;
            break;
        default:
            break;
    }
    return score;
}

VkPhysicalDevice Instance::getBestDevice()
{
    uint32_t physicalDeviceCount = 1;
    uint32_t const req_count = physicalDeviceCount;
    VK_CHECK(vkEnumeratePhysicalDevices(instance_, &physicalDeviceCount, nullptr));
    assert(physicalDeviceCount > 0 && "No GPU found");

    std::vector<VkPhysicalDevice> physicalDevices;
    physicalDevices.resize(physicalDeviceCount);
    VK_CHECK(vkEnumeratePhysicalDevices(instance_, &physicalDeviceCount, physicalDevices.data()));

    assert(physicalDevices.size() > 0 && "No GPU found");

    std::multimap<uint32_t, VkPhysicalDevice> ratings;
    for (auto it = physicalDevices.begin(); it != physicalDevices.end(); ++it)
        ratings.insert(std::make_pair(GetScore(*it), *it));

    return ratings.rbegin()->second;
}

} // yu::vk