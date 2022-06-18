//
// Created by 秋鱼 on 2022/6/17.
//

#include <common/common.hpp>
#include <logger.hpp>
#include "pipeline_builder.hpp"
#include "initializers.hpp"
#include "error.hpp"

namespace yu::vk {

void PipelineBuilder::create(const VulkanDevice& device)
{
    device_ = &device;
}

void PipelineBuilder::destroy()
{
    for (auto& module : shader_modules_) {
        vkDestroyShaderModule(device_->getHandle(), module, nullptr);
    }
}

void PipelineBuilder::setShader(const std::vector<std::string_view>& shaders)
{
    for (auto shader : shaders) {
        auto shaderModule = LoadShader(GetSpvShaderFile(shader.data()), device_->getHandle());
        assert(shaderModule != VK_NULL_HANDLE);

        shader_modules_.push_back(shaderModule);

        auto shaderStage = pipelineShaderStageCreateInfo();
        shaderStage.stage = GetShaderType(shader);
        shaderStage.module = shaderModule;
        shaderStage.pName = "main";

        shader_stages_.push_back(shaderStage);
    }
}

void PipelineBuilder::setVertexInputState(const std::vector<VkVertexInputBindingDescription>& binding,
                                          const std::vector<VkVertexInputAttributeDescription>& layout)
{
    vertex_input_state_ = pipelineVertexInputStateCreateInfo();
    vertex_input_state_.pNext = nullptr;
    vertex_input_state_.flags = 0;
    vertex_input_state_.vertexBindingDescriptionCount = static_cast<uint32_t>(binding.size());
    vertex_input_state_.pVertexBindingDescriptions = binding.data();
    vertex_input_state_.vertexAttributeDescriptionCount = static_cast<uint32_t>(layout.size());
    vertex_input_state_.pVertexAttributeDescriptions = layout.data();
}

void PipelineBuilder::setInputAssemblyState(VkBool32 bPrimitiveRestart, VkPrimitiveTopology topology)
{
    input_assembly_state_ = pipelineInputAssemblyStateCreateInfo(topology, 0, bPrimitiveRestart);

}

void PipelineBuilder::setRasterizationState(VkCullModeFlags cullMode, VkPolygonMode polygonMode, VkFrontFace frontFace)
{
    raster_state_ = pipelineRasterizationStateCreateInfo(polygonMode, cullMode, frontFace);
}

void PipelineBuilder::setColorBlendState(const std::vector<VkPipelineColorBlendAttachmentState>& attStates)
{
    color_blend_state_state_ =
        pipelineColorBlendStateCreateInfo(static_cast<uint32_t>(attStates.size()), attStates.data());
    color_blend_state_state_.logicOpEnable = VK_FALSE;
    color_blend_state_state_.logicOp = VK_LOGIC_OP_NO_OP;
    color_blend_state_state_.blendConstants[0] = 1.0f;
    color_blend_state_state_.blendConstants[1] = 1.0f;
    color_blend_state_state_.blendConstants[2] = 1.0f;
    color_blend_state_state_.blendConstants[3] = 1.0f;
}

void PipelineBuilder::setDynamicState(const std::vector<VkDynamicState>& dynamicStates)
{
    dynamic_state_ = pipelineDynamicStateCreateInfo(dynamicStates);
}

void PipelineBuilder::setViewPortState()
{
    viewport_state_ = pipelineViewportStateCreateInfo();
    viewport_state_.viewportCount = 1;
    viewport_state_.scissorCount = 1;
    viewport_state_.pScissors = nullptr;
    viewport_state_.pViewports = nullptr;
}

void PipelineBuilder::setDepthStencilState()
{
    depth_stencil_state_ = pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE);
}

void PipelineBuilder::setMultisampleState(VkSampleCountFlagBits sampleCount)
{
    multisample_state_ = pipelineMultisampleStateCreateInfo(sampleCount);
}

void PipelineBuilder::createPipeline(VkRenderPass renderPass, VkPipelineLayout pipelineLayout, VkPipeline& pipeline)
{
    if (shader_stages_.empty()) {
        LOG_FATAL("No available shader, set the shader before create pipeline");
    }

    if (EntityNotSet(vertex_input_state_)) {
        vertex_input_state_ = pipelineVertexInputStateCreateInfo();
        vertex_input_state_.vertexBindingDescriptionCount = 0;
        vertex_input_state_.vertexAttributeDescriptionCount = 0;
    }

    if (EntityNotSet(input_assembly_state_)) {
        setInputAssemblyState();
    }

    if (EntityNotSet(raster_state_)) {
        setRasterizationState();
    }

    std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates(1);
    blendAttachmentStates[0] = pipelineColorBlendAttachmentState(static_cast<VkColorComponentFlagBits>(0xf), VK_FALSE);
    blendAttachmentStates[0].alphaBlendOp = VK_BLEND_OP_ADD;
    blendAttachmentStates[0].colorBlendOp = VK_BLEND_OP_ADD;
    blendAttachmentStates[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blendAttachmentStates[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttachmentStates[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blendAttachmentStates[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    if (EntityNotSet(color_blend_state_state_)) {
        setColorBlendState(blendAttachmentStates);
    }

    std::vector<VkDynamicState> dynamicStateEnables = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_BLEND_CONSTANTS
    };
    if (EntityNotSet(dynamic_state_)) {

        setDynamicState(dynamicStateEnables);
    }

    if (EntityNotSet(viewport_state_)) {
        setViewPortState();
    }

    if (EntityNotSet(depth_stencil_state_)) {
        setDepthStencilState();
    }

    if (EntityNotSet(multisample_state_)) {
        setMultisampleState();
    }

    auto pipelineInfo = pipelineCreateInfo();
    pipelineInfo.stageCount = static_cast<uint32_t>(shader_stages_.size());
    pipelineInfo.pStages = shader_stages_.data();
    pipelineInfo.pVertexInputState = &vertex_input_state_;
    pipelineInfo.pInputAssemblyState = &input_assembly_state_;
    pipelineInfo.pRasterizationState = &raster_state_;
    pipelineInfo.pTessellationState = nullptr;
    pipelineInfo.pColorBlendState = &color_blend_state_state_;
    pipelineInfo.pMultisampleState = &multisample_state_;
    pipelineInfo.pViewportState = &viewport_state_;
    pipelineInfo.pDepthStencilState = &depth_stencil_state_;
    pipelineInfo.pDynamicState = &dynamic_state_;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    VK_CHECK(vkCreateGraphicsPipelines(device_->getHandle(),
                                       device_->getPipelineCache(),
                                       1,
                                       &pipelineInfo,
                                       nullptr,
                                       &pipeline));
}

} // yu::vk