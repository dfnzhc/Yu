//
// Created by 秋鱼 on 2022/6/6.
//

#pragma once

#include "device.hpp"

namespace yu::vk {

class VulkanPipeline
{
public:
    void create(const VulkanDevice& device,
                VkRenderPass renderPass,
                const std::vector<std::string_view>& shaders,
                VkDescriptorSetLayout descriptorSetLayout,
                VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT,
                VkPipelineColorBlendStateCreateInfo* blendDesc = nullptr);

    void updatePipeline(VkRenderPass renderPass,
                        VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT,
                        VkPipelineColorBlendStateCreateInfo* blendDesc = nullptr);

    void destroy();

    void draw(VkCommandBuffer cmdBuffer, VkDescriptorBufferInfo* pConstantBuffer = nullptr, VkDescriptorSet descriptorSet = nullptr);

private:
    void loadShader(std::string_view shaderFile);

private:
    const VulkanDevice* device_ = nullptr;
    std::vector<VkShaderModule> shader_modules_;
    std::vector<VkPipelineShaderStageCreateInfo> shader_stages_;

    VkPipeline pipeline_{};
    VkPipelineLayout pipeline_layout_{};
};

} // yu::vk