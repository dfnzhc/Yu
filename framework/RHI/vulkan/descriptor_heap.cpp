//
// Created by 秋鱼 on 2022/6/14.
//

#include "descriptor_heap.hpp"
#include "error.hpp"
#include "initializers.hpp"

namespace yu::vk {

void DescriptorHeap::create(const VulkanDevice& device,
                            uint32_t cbvDescriptorCount,
                            uint32_t srvDescriptorCount,
                            uint32_t samplerDescriptorCount,
                            uint32_t uavDescriptorCount
)
{
    device_ = &device;
    allocated_descriptor_count_ = 0;

    const VkDescriptorPoolSize type_count[] =
        {
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, cbvDescriptorCount},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, cbvDescriptorCount},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, srvDescriptorCount},
            {VK_DESCRIPTOR_TYPE_SAMPLER, samplerDescriptorCount},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, uavDescriptorCount}
        };

    VkDescriptorPoolCreateInfo descriptor_pool = {};
    descriptor_pool.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool.pNext = nullptr;
    descriptor_pool.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    descriptor_pool.maxSets = 8000;
    descriptor_pool.poolSizeCount = _countof(type_count);
    descriptor_pool.pPoolSizes = type_count;

    VK_CHECK(vkCreateDescriptorPool(device_->getHandle(), &descriptor_pool, nullptr, &descriptor_pool_));
}

void DescriptorHeap::destroy()
{
    if (descriptor_pool_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device_->getHandle(), descriptor_pool_, nullptr);
        descriptor_pool_ = VK_NULL_HANDLE;
    }
}

void DescriptorHeap::allocDescriptor(VkDescriptorSetLayout descriptorLayout, VkDescriptorSet* pDescriptorSet)
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto alloc_info = descriptorSetAllocateInfo(descriptor_pool_, &descriptorLayout, 1);
    VK_CHECK(vkAllocateDescriptorSets(device_->getHandle(), &alloc_info, pDescriptorSet));

    allocated_descriptor_count_ += 1;
}

void DescriptorHeap::allocDescriptor(int size, const VkSampler* pSamplers, VkDescriptorSetLayout* pDescSetLayout, VkDescriptorSet* pDescriptorSet)
{

    std::vector<VkDescriptorSetLayoutBinding> layoutBindings(size);
    for (int i = 0; i < size; i++) {
        layoutBindings[i].binding = i;
        layoutBindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        layoutBindings[i].descriptorCount = 1;
        layoutBindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        layoutBindings[i].pImmutableSamplers = (pSamplers != nullptr) ? &pSamplers[i] : nullptr;
    }

    createDescriptorSetLayoutAndAllocDescriptorSet(&layoutBindings, pDescSetLayout, pDescriptorSet);
}

void DescriptorHeap::allocDescriptor(std::vector<uint32_t>& descriptorCounts,
                                     const VkSampler* pSamplers,
                                     VkDescriptorSetLayout* pDescSetLayout,
                                     VkDescriptorSet* pDescriptorSet)
{
    std::vector<VkDescriptorSetLayoutBinding> layoutBindings(descriptorCounts.size());
    for (int i = 0; i < descriptorCounts.size(); i++) {
        layoutBindings[i].binding = i;
        layoutBindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        layoutBindings[i].descriptorCount = descriptorCounts[i];
        layoutBindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        layoutBindings[i].pImmutableSamplers = (pSamplers != nullptr) ? &pSamplers[i] : nullptr;
    }

    createDescriptorSetLayoutAndAllocDescriptorSet(&layoutBindings, pDescSetLayout, pDescriptorSet);
}

void DescriptorHeap::createDescriptorSetLayout(std::vector<VkDescriptorSetLayoutBinding>* pDescriptorLayoutBinding,
                                               VkDescriptorSetLayout* pDescSetLayout)
{
    auto descriptor_layout = descriptorSetLayoutCreateInfo(pDescriptorLayoutBinding->data(),
                                                           static_cast<uint32_t>(pDescriptorLayoutBinding->size()));

    VK_CHECK(vkCreateDescriptorSetLayout(device_->getHandle(), &descriptor_layout, nullptr, pDescSetLayout));
}

void DescriptorHeap::createDescriptorSetLayoutAndAllocDescriptorSet(std::vector<VkDescriptorSetLayoutBinding>* pDescriptorLayoutBinding,
                                                                    VkDescriptorSetLayout* pDescSetLayout,
                                                                    VkDescriptorSet* pDescriptorSet)
{
    auto descriptor_layout = descriptorSetLayoutCreateInfo(pDescriptorLayoutBinding->data(),
                                                           static_cast<uint32_t>(pDescriptorLayoutBinding->size()));

    VK_CHECK(vkCreateDescriptorSetLayout(device_->getHandle(), &descriptor_layout, nullptr, pDescSetLayout));

    return allocDescriptor(*pDescSetLayout, pDescriptorSet);
}

void DescriptorHeap::freeDescriptor(VkDescriptorSet descriptorSet)
{
    allocated_descriptor_count_--;
    vkFreeDescriptorSets(device_->getHandle(), descriptor_pool_, 1, &descriptorSet);
}

} // yu::vk
