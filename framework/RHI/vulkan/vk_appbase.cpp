//
// Created by 秋鱼 on 2022/5/11.
//

#include "vk_appbase.hpp"
#include "vk_error.hpp"
#include "vk_utils.hpp"
#include <vulkan/vulkan_win32.h>

namespace ST::VK {

bool AppBase::prepare(Platform& platform)
{
    if (!Application::prepare(platform)) {
        return false;
    }

    initVulkan();

    return true;
}

void AppBase::update(float delta_time)
{
    Application::update(delta_time);
}

void AppBase::finish()
{
    vkDeviceWaitIdle(device_);
    
    delete vulkan_device_;
    
    if (settings_.validation) {
        DestroyDebugMessenger(instance_, Debug_messenger_, nullptr);
    }

    vkDestroyInstance(instance_, nullptr);
}

bool AppBase::resize(const uint32_t width, const uint32_t height)
{

    return true;
}

void AppBase::input_event(const InputEvent& input_event)
{
    Application::input_event(input_event);
}

void AppBase::initVulkan()
{
    LOG_INFO("Init vulkan...")
    // first create vulkan instance
    createInstance();

    // physical device
    LOG_INFO("\tEnumerate physical devices...");
    uint32_t gpuCount = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(instance_, &gpuCount, nullptr));
    if (gpuCount == 0) {
        throw std::runtime_error("No device with Vulkan support found");
    }
    // Enumerate devices
    std::vector<VkPhysicalDevice> physicalDevices(gpuCount);
    VK_CHECK(vkEnumeratePhysicalDevices(instance_, &gpuCount, physicalDevices.data()));

    const int gpuIdx = 0;
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevices[gpuIdx], &deviceProperties);
    LOG_INFO("\t\tSelected device: {}", deviceProperties.deviceName);
    LOG_INFO("\t\tTYPE: {}", PhysicalDeviceTypeString(deviceProperties.deviceType));
    LOG_INFO("\t\tAPI: {}.{}.{}",
             (deviceProperties.apiVersion >> 22),
             ((deviceProperties.apiVersion >> 12) & 0x3ff),
             (deviceProperties.apiVersion & 0xfff));

    physical_device_ = physicalDevices[gpuIdx];
    // 存储物理设备的属性（包括限制）、特征和内存属性（以便实例可以对照它们进行检查）
    vkGetPhysicalDeviceProperties(physical_device_, &deviceProperties);
    vkGetPhysicalDeviceFeatures(physical_device_, &device_features_);
    vkGetPhysicalDeviceMemoryProperties(physical_device_, &device_memory_properties_);

    getDeviceEnabledFeatures();

    // 创建Vulkan设备
    // 这由一个单独的类来处理，该类得到一个逻辑设备的表示，并封装了与设备有关的功能
    vulkan_device_ = new VulkanDevice(physical_device_);
    VK_CHECK(vulkan_device_->createLogicalDevice(enabled_features_, enabled_device_extensions_, device_create_pNext));

    device_ = vulkan_device_->logicalDevice;

    // 从设备上获取图形队列
    vkGetDeviceQueue(device_, vulkan_device_->queueFamilyIndices.graphics, 0, &queue_);
    
    // 寻找一个合适的深度格式
	VkBool32 validDepthFormat = GetSupportedDepthFormat(physical_device_, &depth_format_);
	assert(validDepthFormat);
}

void AppBase::createInstance()
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
        if (settings_.validation) {
            instanceExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
        instanceCreateInfo.enabledExtensionCount = (uint32_t) instanceExtensions.size();
        instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();
    }

    const char* validationLayerName = "VK_LAYER_KHRONOS_validation";
    if (settings_.validation) {
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

    if (settings_.validation) {
        SetupDebugMessenger(instance_, nullptr, &Debug_messenger_);
    }
}

} // namespace ST