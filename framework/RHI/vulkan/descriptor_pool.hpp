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

    void allocDescriptor(VkDescriptorSetLayout descriptorLayout, VkDescriptorSet* pDescriptorSet);
    void allocDescriptor(int size, const VkSampler* pSamplers, VkDescriptorSetLayout* pDescSetLayout, VkDescriptorSet* pDescriptorSet);
    void allocDescriptor(std::vector<uint32_t>& descriptorCounts,
                         const VkSampler* pSamplers,
                         VkDescriptorSetLayout* pDescSetLayout,
                         VkDescriptorSet* pDescriptorSet);

    void createDescriptorSetLayout(std::vector<VkDescriptorSetLayoutBinding>* pDescriptorLayoutBinding,
                                   VkDescriptorSetLayout* pDescSetLayout);
    void createDescriptorSetLayoutAndAllocDescriptorSet(std::vector<VkDescriptorSetLayoutBinding>* pDescriptorLayoutBinding,
                                                        VkDescriptorSetLayout* pDescSetLayout,
                                                        VkDescriptorSet* pDescriptorSet);

    void freeDescriptor(VkDescriptorSet descriptorSet);

private:
    const VulkanDevice* device_ = nullptr;

    VkDescriptorPool descriptor_pool_{};

    std::mutex mutex_;
    uint32_t allocated_descriptor_count_{};

};

} // yu::vk