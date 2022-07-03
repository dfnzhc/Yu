//
// Created by 秋鱼 on 2022/6/1.
//

#pragma once

#include "instance_properties.hpp"
namespace yu::vk {

bool CheckDebugUtilsInstanceEXT(InstanceProperties& ip);

void SetupDebugMessenger(VkInstance instance);
void DestroyDebugMessenger(VkInstance instance);

class DebugUtil
{
public:
    DebugUtil() = default;
    DebugUtil(VkDevice device);

    void setup(VkDevice device) { device_ = device; }

    void setObjectName(const uint64_t object, const std::string& name, VkObjectType t);

    void setName(VkBuffer object, const std::string& name) { setObjectName((uint64_t) object, name, VK_OBJECT_TYPE_BUFFER); }
    void setName(VkBufferView object, const std::string& name) { setObjectName((uint64_t) object, name, VK_OBJECT_TYPE_BUFFER_VIEW); }
    void setName(VkCommandBuffer object, const std::string& name) { setObjectName((uint64_t) object, name, VK_OBJECT_TYPE_COMMAND_BUFFER); }
    void setName(VkCommandPool object, const std::string& name) { setObjectName((uint64_t) object, name, VK_OBJECT_TYPE_COMMAND_POOL); }
    void setName(VkDescriptorPool object, const std::string& name) { setObjectName((uint64_t) object, name, VK_OBJECT_TYPE_DESCRIPTOR_POOL); }
    void setName(VkDescriptorSet object, const std::string& name) { setObjectName((uint64_t) object, name, VK_OBJECT_TYPE_DESCRIPTOR_SET); }
    void setName(VkDescriptorSetLayout object, const std::string& name)
    {
        setObjectName((uint64_t) object, name, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT);
    }
    void setName(VkDevice object, const std::string& name) { setObjectName((uint64_t) object, name, VK_OBJECT_TYPE_DEVICE); }
    void setName(VkDeviceMemory object, const std::string& name) { setObjectName((uint64_t) object, name, VK_OBJECT_TYPE_DEVICE_MEMORY); }
    void setName(VkFramebuffer object, const std::string& name) { setObjectName((uint64_t) object, name, VK_OBJECT_TYPE_FRAMEBUFFER); }
    void setName(VkImage object, const std::string& name) { setObjectName((uint64_t) object, name, VK_OBJECT_TYPE_IMAGE); }
    void setName(VkImageView object, const std::string& name) { setObjectName((uint64_t) object, name, VK_OBJECT_TYPE_IMAGE_VIEW); }
    void setName(VkPipeline object, const std::string& name) { setObjectName((uint64_t) object, name, VK_OBJECT_TYPE_PIPELINE); }
    void setName(VkPipelineLayout object, const std::string& name) { setObjectName((uint64_t) object, name, VK_OBJECT_TYPE_PIPELINE_LAYOUT); }
    void setName(VkQueryPool object, const std::string& name) { setObjectName((uint64_t) object, name, VK_OBJECT_TYPE_QUERY_POOL); }
    void setName(VkQueue object, const std::string& name) { setObjectName((uint64_t) object, name, VK_OBJECT_TYPE_QUEUE); }
    void setName(VkRenderPass object, const std::string& name) { setObjectName((uint64_t) object, name, VK_OBJECT_TYPE_RENDER_PASS); }
    void setName(VkSampler object, const std::string& name) { setObjectName((uint64_t) object, name, VK_OBJECT_TYPE_SAMPLER); }
    void setName(VkSemaphore object, const std::string& name) { setObjectName((uint64_t) object, name, VK_OBJECT_TYPE_SEMAPHORE); }
    void setName(VkShaderModule object, const std::string& name) { setObjectName((uint64_t) object, name, VK_OBJECT_TYPE_SHADER_MODULE); }
    void setName(VkSwapchainKHR object, const std::string& name) { setObjectName((uint64_t) object, name, VK_OBJECT_TYPE_SWAPCHAIN_KHR); }

#if VK_NV_ray_tracing
    void setName(VkAccelerationStructureNV object, const std::string& name)
    {
        setObjectName((uint64_t) object, name, VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV);
    }
#endif
#if VK_KHR_acceleration_structure
    void setName(VkAccelerationStructureKHR object, const std::string& name)
    {
        setObjectName((uint64_t) object, name, VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR);
    }
#endif

    void beginLabel(VkCommandBuffer cmdBuf, const std::string& label);
    void endLabel(VkCommandBuffer cmdBuf);
    void insertLabel(VkCommandBuffer cmdBuf, const std::string& label);
private:
    VkDevice device_{};
};

} // namespace yu::vk
