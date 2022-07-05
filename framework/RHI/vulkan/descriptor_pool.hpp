//
// Created by 秋鱼 on 2022/6/14.
//

#pragma once

#include "device.hpp"
namespace yu::vk {

class DescriptorPool
{
public:
    void create(const VulkanDevice& device,
                uint32_t cbvDescriptorCount,
                uint32_t srvDescriptorCount,
                uint32_t samplerDescriptorCount,
                uint32_t uavDescriptorCount
    );

    void destroy();

    /**
     * @brief 通过描述符的布局创建描述符集
     */
    void allocDescriptor(VkDescriptorSetLayout descriptorLayout, VkDescriptorSet* pDescriptorSet);
    
    /**
     * @brief 按照传入的采样器创建绑定信息，并创建描述符布局和描述符集
     */
    void allocDescriptor(int size, const VkSampler* pSamplers, VkDescriptorSetLayout* pDescSetLayout, VkDescriptorSet* pDescriptorSet);
    
    /**
     * @brief 按照传入的采样器创建绑定信息，并创建描述符布局和描述符集，一个绑定点可能有多个描述符
     */    
    void allocDescriptor(std::vector<uint32_t>& descriptorCounts,
                         const VkSampler* pSamplers,
                         VkDescriptorSetLayout* pDescSetLayout,
                         VkDescriptorSet* pDescriptorSet);

    
    /**
     * @brief 通过绑定信息创建描述符布局
     */
    void createDescriptorSetLayout(std::vector<VkDescriptorSetLayoutBinding>* pDescriptorLayoutBinding,
                                   VkDescriptorSetLayout* pDescSetLayout);
    
    /**
     * @brief 通过绑定信息创建描述符布局，同时创建描述符集
     */    
    void createDescriptorSetLayoutAndAllocDescriptorSet(std::vector<VkDescriptorSetLayoutBinding>* pDescriptorLayoutBinding,
                                                        VkDescriptorSetLayout* pDescSetLayout,
                                                        VkDescriptorSet* pDescriptorSet);

    void freeDescriptor(VkDescriptorSet descriptorSet);

private:
    const VulkanDevice* device_ = nullptr;

    VkDescriptorPool descriptor_pool_{};

    std::mutex mutex_{};
    uint32_t allocated_descriptor_count_{};
};

} // yu::vk