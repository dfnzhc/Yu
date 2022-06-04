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

namespace yu::vk {

class Device
{
public:
    Device() = default;
    ~Device();
    
    void init(VkInstance instance, VkPhysicalDevice physicalDevice, VkQueueFlags requestedQueueTypes = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);
    void cleanup();
    
    VkDevice getHandle() const { return device_; }
    DeviceProperties getProperties() const { return properties_; }
    
#ifdef USE_VMA
    VmaAllocator getAllocator() const { return allocator_; }
#endif

private:
    void setEssentialExtensions();

private:
    DeviceProperties properties_;

    VkDevice device_{VK_NULL_HANDLE};
    
    VkQueue graphics_queue_{};
    uint32_t graphics_queue_index_{};
    
    VkQueue compute_queue_{};
    uint32_t compute_queue_index_{};
    
    VkQueue transfer_queue_{};
    uint32_t transfer_queue_index_{};
    
#ifdef USE_VMA
    VmaAllocator allocator_ = nullptr;
#endif
};

} // yu::vk