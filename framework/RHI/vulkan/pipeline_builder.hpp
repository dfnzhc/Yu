//
// Created by 秋鱼 on 2022/6/17.
//

#pragma once

#include "device.hpp"

namespace yu::vk {

class PipelineBuilder
{
public:
    PipelineBuilder() = default;
    virtual ~PipelineBuilder() = default;

    void create(const VulkanDevice& device);
    void destroy();

    void setShader(const std::vector<std::string_view>& shaders);
    void setVertexInputState(const std::vector<VkVertexInputBindingDescription>& binding,
                             const std::vector<VkVertexInputAttributeDescription>& layout);
    void setInputAssemblyState(VkBool32 bPrimitiveRestart = VK_FALSE,
                               VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    void setRasterizationState(VkCullModeFlags cullMode = VK_CULL_MODE_NONE,
                               VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL,
                               VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE);

    void setColorBlendState(const std::vector<VkPipelineColorBlendAttachmentState>& attStates);
    void setDynamicState(const std::vector<VkDynamicState>& dynamicStates);
    virtual void setViewPortState();
    virtual void setDepthStencilState(VkBool32 depthTestEnable = VK_TRUE,
                                      VkBool32 depthWriteEnable = VK_TRUE,
                                      VkCompareOp depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL);
    void setMultisampleState(VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT);

    void createPipeline(VkRenderPass renderPass, VkPipelineLayout pipelineLayout, VkPipeline& pipeline);
private:

private:
    const VulkanDevice* device_ = nullptr;

    std::vector<VkShaderModule> shader_modules_;
    std::vector<VkPipelineShaderStageCreateInfo> shader_stages_;

    VkPipelineVertexInputStateCreateInfo vertex_input_state_{};
    VkPipelineInputAssemblyStateCreateInfo input_assembly_state_{};

    VkPipelineRasterizationStateCreateInfo raster_state_{};
    VkPipelineColorBlendStateCreateInfo color_blend_state_state_{};

    VkPipelineDynamicStateCreateInfo dynamic_state_{};
    VkPipelineViewportStateCreateInfo viewport_state_{};

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_{};
    VkPipelineMultisampleStateCreateInfo multisample_state_{};
};

} // yu::vk