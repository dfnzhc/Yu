﻿//
// Created by 秋鱼 on 2022/6/6.
//

#pragma once

#include "device.hpp"
#include "pipeline_builder.hpp"

namespace yu::vk {

class VulkanPipeline
{
public:
    void create(const VulkanDevice& device,
                VkRenderPass renderPass,
                VkDescriptorSetLayout descriptorSetLayout,
                PipelineBuilder& pipelineBuilder);
    void destroy();
    
    [[deprecated("Hard code vertices in shader")]]
    void draw(VkCommandBuffer cmdBuffer,
              VkDescriptorBufferInfo* pConstantBuffer = nullptr,
              VkDescriptorSet descriptorSet = nullptr);

    void draw(VkCommandBuffer cmdBuffer,
              uint32_t vertexCount,
              VkDescriptorBufferInfo* pVertexBuffer,
              VkDescriptorBufferInfo* pConstantBuffer = nullptr,
              VkDescriptorSet descriptorSet = nullptr);
private:
    const VulkanDevice* device_ = nullptr;

    VkPipeline pipeline_{};
    VkPipelineLayout pipeline_layout_{};
};

} // yu::vk