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

    VkQueue getGraphicsQueue() const { return graphics_queue_; }
    uint32_t getGraphicsQueueIndex() const { return graphics_queue_index_; }

    VkQueue getComputeQueue() const { return compute_queue_; }
    uint32_t getComputeQueueIndex() const { return compute_queue_index_; }

    VkQueue getPresentQueue() const { return present_queue_; }
    uint32_t getPresentQueueIndex() const { return present_queue_index_; }

#ifdef USE_VMA
    VmaAllocator getAllocator() const { return allocator_; }
#endif

    // pipeline cache
    void createPipelineCache();
    void destroyPipelineCache();
    VkPipelineCache getPipelineCache() const { return pipeline_cache_; }

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

    VkPipelineCache pipeline_cache_{};
    
#ifdef USE_VMA
    VmaAllocator allocator_ = nullptr;
#endif
};

} // yu::vk