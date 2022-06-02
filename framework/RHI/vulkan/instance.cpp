//
// Created by 秋鱼 on 2022/6/1.
//

#include <logger.hpp>
#include "instance.hpp"
#include "initializers.hpp"
#include "error.hpp"
#include "ext_debug.hpp"

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

    // set the extensions and layers
    if (properties_.enabled_validation) {
        CheckDebugUtilsInstanceEXT(properties_);
    }

    auto instCI = instanceCreateInfo();
    instCI.pNext = nullptr;
    instCI.pApplicationInfo = &appInfo;
    instCI.enabledLayerCount = static_cast<uint32_t>(properties_.enabled_layers.size());
    instCI.ppEnabledLayerNames = properties_.enabled_layers.data();
    instCI.enabledExtensionCount = static_cast<uint32_t>(properties_.enabled_extensions.size());
    instCI.ppEnabledExtensionNames = properties_.enabled_extensions.data();

    VK_CHECK(vkCreateInstance(&instCI, nullptr, &handle_));

    // create debug messenger
    SetupDebugMessenger(handle_);
}

Instance::~Instance()
{
    // destroy debug messenger
    DestroyDebugMessenger(handle_);

    if (handle_ != VK_NULL_HANDLE) {
        vkDestroyInstance(handle_, nullptr);
    }
}

} // yu::vk