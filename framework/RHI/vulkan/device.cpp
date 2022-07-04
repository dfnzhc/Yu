//
// Created by 秋鱼 on 2022/6/3.
//

#include "device.hpp"
#include "ext_float.hpp"
#include "ext_hdr.hpp"
#include "initializers.hpp"
#include "error.hpp"
#include "instance.hpp"
#include "ext_raytracing.hpp"

#ifdef USE_VMA
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#endif

#include <logger.hpp>

namespace yu::vk {

VulkanDevice::~VulkanDevice()
{
    destroy();
}

void VulkanDevice::create(const VulkanInstance& instance)
{
    properties_.init(instance.getBestDevice());
    setEssentialExtensions();

    // 获取设备的队列信息
    auto queueCreateInfos = getDeviceQueueInfos(instance.getSurface());

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
    if (graphics_queue_index_ == present_queue_index_) {
        present_queue_ = graphics_queue_;
    } else {
        vkGetDeviceQueue(device_, present_queue_index_, 0, &present_queue_);
    }

    if (graphics_queue_index_ == compute_queue_index_) {
        compute_queue_ = graphics_queue_;
    } else {
        vkGetDeviceQueue(device_, compute_queue_index_, 0, &compute_queue_);
    }
    
    // 创建命令池
    {
        auto cmdPoolInfo = commandPoolCreateInfo();
        cmdPoolInfo.queueFamilyIndex = graphics_queue_index_;
        cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        VK_CHECK(vkCreateCommandPool(device_, &cmdPoolInfo, nullptr, &command_pool_));
    }
    
    // 创建 VMA(分配器)
#ifdef USE_VMA
    VmaAllocatorCreateInfo allocator_info = {};
    allocator_info.physicalDevice = properties_.physical_device;
    allocator_info.device = device_;
    allocator_info.instance = instance.getHandle();
    vmaCreateAllocator(&allocator_info, &allocator_);
#endif
}

void VulkanDevice::destroy()
{
#ifdef USE_VMA
    if (allocator_ != nullptr) {
        vmaDestroyAllocator(allocator_);
        allocator_ = nullptr;
    }
#endif

    destroyPipelineCache();

    if (command_pool_ != VK_NULL_HANDLE) {
        vkDestroyCommandPool(device_, command_pool_, nullptr);
        command_pool_ = VK_NULL_HANDLE;
    }
    
    if (device_ != VK_NULL_HANDLE) {
        vkDestroyDevice(device_, nullptr);
        device_ = VK_NULL_HANDLE;
    }
}

void VulkanDevice::setEssentialExtensions()
{
    CheckFP16DeviceEXT(properties_);
    CheckHDRDeviceEXT(properties_);
//    CheckRTDeviceEXT(properties_);

    properties_.addExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    properties_.addExtension(VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME);
}

std::vector<VkDeviceQueueCreateInfo> VulkanDevice::getDeviceQueueInfos(VkSurfaceKHR surface)
{
    // 逻辑设备创建所需要的队列
    const float defaultQueuePriority(0.0f);
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    // 查询 graphics 和 present 队列的索引，使用同时支持两者的队列
    for (uint32_t i = 0; i < properties_.queue_family_count; i++) {
        if (properties_.queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            if (graphics_queue_index_ == UINT32_MAX)
                graphics_queue_index_ = i;

            VkBool32 supportsPresent;
            vkGetPhysicalDeviceSurfaceSupportKHR(properties_.physical_device, i, surface, &supportsPresent);
            if (supportsPresent == VK_TRUE) {
                graphics_queue_index_ = i;
                present_queue_index_ = i;
                break;
            }
        }
    }

    {
        auto queue_info = deviceQueueCreateInfo(graphics_queue_index_);
        queue_info.pQueuePriorities = &defaultQueuePriority;

        queueCreateInfos.push_back(queue_info);
    }


    // 如果没有同时支持 graphics 和 present 的队列，那么就创建独立的 present 队列
    if (present_queue_index_ == UINT32_MAX) {
        for (uint32_t i = 0; i < properties_.queue_family_count; i++) {
            VkBool32 supportsPresent;
            vkGetPhysicalDeviceSurfaceSupportKHR(properties_.physical_device, i, surface, &supportsPresent);
            if (supportsPresent == VK_TRUE) {
                present_queue_index_ = i;

                if (present_queue_index_ != graphics_queue_index_) {
                    auto queue_info = deviceQueueCreateInfo(present_queue_index_);
                    queue_info.pQueuePriorities = &defaultQueuePriority;

                    queueCreateInfos.push_back(queue_info);
                }
                break;
            }
        }
    }

    // 创建计算队列
    if (compute_queue_index_ == UINT32_MAX) {
        for (uint32_t i = 0; i < properties_.queue_family_count; i++) {
            if (properties_.queue_family_properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                compute_queue_index_ = i;
                break;
            }
        }
    }

    if (compute_queue_index_ == UINT32_MAX) {
        compute_queue_index_ = graphics_queue_index_;
    } else if (compute_queue_index_ != graphics_queue_index_) {
        auto queue_info = deviceQueueCreateInfo(compute_queue_index_);
        queue_info.pQueuePriorities = &defaultQueuePriority;

        queueCreateInfos.push_back(queue_info);
    }

    return queueCreateInfos;
}

void VulkanDevice::createPipelineCache()
{
    VkPipelineCacheCreateInfo pipelineCache;
    pipelineCache.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    pipelineCache.pNext = nullptr;
    pipelineCache.initialDataSize = 0;
    pipelineCache.pInitialData = nullptr;
    pipelineCache.flags = 0;
    VK_CHECK(vkCreatePipelineCache(device_, &pipelineCache, nullptr, &pipeline_cache_));
}

void VulkanDevice::destroyPipelineCache()
{
    if (pipeline_cache_ != VK_NULL_HANDLE) {
        vkDestroyPipelineCache(device_, pipeline_cache_, nullptr);
        pipeline_cache_ = VK_NULL_HANDLE;
    }
}

/**
* 从命令池中分配一个命令缓冲区
*
* @param level 新命令缓冲区的级别（主要或次要）
* @param pool 命令池，命令缓冲区将从该池中分配
* @param (Optional) 如果为真，将开始对新的命令缓冲区进行记录（vkBeginCommandBuffer） (Defaults to false)
*
* @return 分配的命令缓冲区的句柄
*/
VkCommandBuffer VulkanDevice::createCommandBuffer(VkCommandBufferLevel level, VkCommandPool pool, bool begin)
{
    VkCommandBufferAllocateInfo cmdBufAllocateInfo = commandBufferAllocateInfo(pool, level, 1);
    VkCommandBuffer cmdBuffer;
    VK_CHECK(vkAllocateCommandBuffers(device_, &cmdBufAllocateInfo, &cmdBuffer));
    // 如果有要求，开始向新的命令缓冲区进行记录
    if (begin) {
        VkCommandBufferBeginInfo cmdBufInfo = commandBufferBeginInfo();
        VK_CHECK(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));
    }
    return cmdBuffer;
}

VkCommandBuffer VulkanDevice::createCommandBuffer(VkCommandBufferLevel level, bool begin)
{
    return createCommandBuffer(level, command_pool_, begin);
}

/**
* 完成命令缓冲区的记录，并将其提交给队列
*
* @param commandBuffer 要刷新的命令缓冲区
* @param queue 要提交命令缓冲区的队列
* @param pool 创建命令缓冲区的命令池
* @param free (Optional) 一旦命令缓冲区被提交就释放它 (默认为 true)
*
* @note 提交命令缓冲区的队列必须与它所分配的池子来自同一个队列集合索引
* @note 使用 fence 来确保命令缓冲区已经完成执行
*/
void VulkanDevice::flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, VkCommandPool pool, bool free)
{
    if (commandBuffer == VK_NULL_HANDLE) {
        return;
    }

    VK_CHECK(vkEndCommandBuffer(commandBuffer));

    VkSubmitInfo info = submitInfo();
    info.commandBufferCount = 1;
    info.pCommandBuffers = &commandBuffer;
    // 创建 fence 以确保命令缓冲区已经执行完毕
    VkFenceCreateInfo fenceInfo = fenceCreateInfo();
    VkFence fence;
    VK_CHECK(vkCreateFence(device_, &fenceInfo, nullptr, &fence));
    // 提交到队列中
    VK_CHECK(vkQueueSubmit(queue, 1, &info, fence));
    // 等待 fence 发出信号，表明命令缓冲区已经执行完毕
    VK_CHECK(vkWaitForFences(device_, 1, &fence, VK_TRUE, UINT64_MAX));
    vkDestroyFence(device_, fence, nullptr);
    if (free) {
        vkFreeCommandBuffers(device_, pool, 1, &commandBuffer);
    }
}

void VulkanDevice::flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free)
{
    return flushCommandBuffer(commandBuffer, queue, command_pool_, free);
}

} // yu::vk
