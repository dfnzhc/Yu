//
// Created by 秋鱼 on 2022/6/3.
//

#include "device.hpp"
#include "ext_float.hpp"
#include "ext_hdr.hpp"
#include "initializers.hpp"
#include "error.hpp"

#ifdef USE_VMA
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#endif

#include <logger.hpp>

namespace yu::vk {

Device::~Device()
{
    cleanup();
}

void Device::init(VkInstance instance, VkPhysicalDevice physicalDevice, VkQueueFlags requestedQueueTypes)
{
    properties_.init(physicalDevice);
    setEssentialExtensions();

    // 逻辑设备创建所需要的队列
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};
    const float defaultQueuePriority(0.0f);

    if (requestedQueueTypes & VK_QUEUE_GRAPHICS_BIT) {
        graphics_queue_index_ = properties_.getQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT);
        auto queueInfo = deviceQueueCreateInfo(graphics_queue_index_);
        queueInfo.pQueuePriorities = &defaultQueuePriority;

        queueCreateInfos.push_back(queueInfo);
    }

    if (requestedQueueTypes & VK_QUEUE_COMPUTE_BIT) {
        compute_queue_index_ = properties_.getQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT);
        auto queueInfo = deviceQueueCreateInfo(compute_queue_index_);
        queueInfo.pQueuePriorities = &defaultQueuePriority;

        queueCreateInfos.push_back(queueInfo);
    } else {
        compute_queue_index_ = graphics_queue_index_;
    }

    if (requestedQueueTypes & VK_QUEUE_TRANSFER_BIT) {
        transfer_queue_index_ = properties_.getQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT);
        auto queueInfo = deviceQueueCreateInfo(transfer_queue_index_);
        queueInfo.pQueuePriorities = &defaultQueuePriority;

        queueCreateInfos.push_back(queueInfo);
    } else {
        transfer_queue_index_ = graphics_queue_index_;
    }

    // 启用支持 16 位浮点的着色器 subgroup 扩展 
    auto shaderSubgroupExtendedType = shaderSubgroupExtendedTypesFeatures();
    shaderSubgroupExtendedType.pNext = properties_.pNext;
    shaderSubgroupExtendedType.shaderSubgroupExtendedTypes = VK_TRUE;

    auto robustness2 = robustness2Features();
    robustness2.pNext = &shaderSubgroupExtendedType;
    robustness2.nullDescriptor = VK_TRUE;

    // 允许绑定空视图
    auto features2 = physicalDeviceFeatures2();
    features2.features = properties_.features;
    features2.pNext = &robustness2;

    auto device_info = deviceCreateInfo();
    device_info.pNext = &features2;
    device_info.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    device_info.pQueueCreateInfos = queueCreateInfos.data();
    device_info.enabledExtensionCount = static_cast<uint32_t>(properties_.enabled_extensions.size());
    device_info.ppEnabledExtensionNames = properties_.enabled_extensions.data();
    device_info.pEnabledFeatures = nullptr;

    VK_CHECK(vkCreateDevice(properties_.physical_device, &device_info, nullptr, &device_));

    // 创建队列
    vkGetDeviceQueue(device_, graphics_queue_index_, 0, &graphics_queue_);
    if (graphics_queue_index_ == compute_queue_index_) {
        compute_queue_ = graphics_queue_;
    } else {
        vkGetDeviceQueue(device_, compute_queue_index_, 0, &compute_queue_);
    }

    if (graphics_queue_index_ == transfer_queue_index_) {
        transfer_queue_ = graphics_queue_;
    } else {
        vkGetDeviceQueue(device_, transfer_queue_index_, 0, &transfer_queue_);
    }

    // 创建 VMA(分配器)
#ifdef USE_VMA
    VmaAllocatorCreateInfo allocator_info = {};
    allocator_info.physicalDevice = properties_.physical_device;
    allocator_info.device = device_;
    allocator_info.instance = instance;
    vmaCreateAllocator(&allocator_info, &allocator_);
#endif
}

void Device::cleanup()
{
#ifdef USE_VMA
    if (allocator_ != nullptr) {
        vmaDestroyAllocator(allocator_);
        allocator_ = nullptr;
    }
#endif

    if (device_ != VK_NULL_HANDLE) {
        vkDestroyDevice(device_, nullptr);
        device_ = VK_NULL_HANDLE;
    }
}

void Device::setEssentialExtensions()
{
    CheckFP16DeviceEXT(properties_);

    CheckHDRDeviceEXT(properties_);

    properties_.addExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    properties_.addExtension(VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME);
}

} // yu::vk
