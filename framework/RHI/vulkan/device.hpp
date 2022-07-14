//
// Created by 秋鱼 on 2022/6/3.
//

#pragma once

//#ifndef USE_VMA
//#define USE_VMA
//#endif

#ifdef USE_VMA
#include <vk_mem_alloc.h>
#endif

#include "device_properties.hpp"
#include "vulkan_utils.hpp"
#include <window.hpp>
#include "instance.hpp"
#include "buffer.hpp"

namespace yu::vk {

class VulkanDevice
{
public:
    VulkanDevice() = default;
    ~VulkanDevice();

    void create(const VulkanInstance& instance);
    void destroy();

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
    VkPipelineCache getPipelineCache() const { return pipeline_cache_; }

    VkResult createBuffer(VkBufferUsageFlags usageFlags,
                          VkMemoryPropertyFlags memoryPropertyFlags,
                          VkDeviceSize size,
                          VkBuffer* buffer,
                          VkDeviceMemory* memory,
                          bool bCopyData,
                          void** pData) const;
    VkResult createBuffer(VkBufferUsageFlags usageFlags,
                          VkMemoryPropertyFlags memoryPropertyFlags,
                          VulkanBuffer* buffer,
                          VkDeviceSize size,
                          void* data = nullptr);
    void copyBuffer(VulkanBuffer* src, VulkanBuffer* dst, VkQueue queue, VkBufferCopy* copyRegion = nullptr);
    

    VkCommandBuffer createCommandBuffer(VkCommandBufferLevel level, VkCommandPool pool, bool begin = false);
    VkCommandBuffer createCommandBuffer(VkCommandBufferLevel level, bool begin = false);
    void flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, VkCommandPool pool, bool free = true);
    void flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free = true);
    
private:
    void setEssentialExtensions();
    std::vector<VkDeviceQueueCreateInfo> getDeviceQueueInfos(VkSurfaceKHR surface);

    void createPipelineCache();
    void destroyPipelineCache();

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
    
    VkCommandPool command_pool_{};
    
#ifdef USE_VMA
    VmaAllocator allocator_ = nullptr;
#endif
};

} // yu::vk