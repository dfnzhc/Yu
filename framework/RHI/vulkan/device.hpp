//
// Created by 秋鱼 on 2022/6/3.
//

#pragma once

#define USE_VMA

#ifdef USE_VMA
#include <vk_mem_alloc.h>
#endif

#include "device_properties.hpp"
#include "vulkan_utils.hpp"
#include <window.hpp>
#include "instance.hpp"

namespace yu::vk {


class VulkanDevice
{
public:
    VulkanDevice() = default;
    ~VulkanDevice();
    
    void init(const VulkanInstance& instance);
    void cleanup();
    
    VkDevice getHandle() const { return device_; }
    DeviceProperties getProperties() const { return properties_; }
    
#ifdef USE_VMA
    VmaAllocator getAllocator() const { return allocator_; }
#endif

private:
    void setEssentialExtensions();
    std::vector<VkDeviceQueueCreateInfo> getDeviceQueueInfos(VkSurfaceKHR surface);

private:
    DeviceProperties properties_;

    VkDevice device_{};
    
    VkQueue graphics_queue_{};
    uint32_t graphics_queue_index_{UINT32_MAX};
    
    VkQueue compute_queue_{};
    uint32_t compute_queue_index_{UINT32_MAX};
    
    VkQueue present_queue_{};
    uint32_t present_queue_index_{UINT32_MAX};
    
#ifdef USE_VMA
    VmaAllocator allocator_ = nullptr;
#endif
};

} // yu::vk