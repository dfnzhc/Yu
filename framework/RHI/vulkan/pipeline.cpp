//
// Created by 秋鱼 on 2022/6/6.
//

#include <logger.hpp>
#include "pipeline.hpp"
#include "initializers.hpp"

#include "common/common.hpp"
#include "error.hpp"

namespace yu::vk {

void VulkanPipeline::create(const VulkanDevice& device,
                            VkRenderPass renderPass,
                            const std::vector<std::string_view>& shaders,
                            VkDescriptorSetLayout descriptorSetLayout,
                            VkSampleCountFlagBits sampleCount,
                            VkPipelineColorBlendStateCreateInfo* blendDesc)
{
    device_ = &device;

    for (auto shader : shaders) {
        loadShader(shader);
    }

    auto pipelineLayoutInfo = pipelineLayoutCreateInfo();
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;
    if (descriptorSetLayout != VK_NULL_HANDLE) {
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
    } else {
        pipelineLayoutInfo.setLayoutCount = 0;
    }

    VK_CHECK(vkCreatePipelineLayout(device_->getHandle(), &pipelineLayoutInfo, nullptr, &pipeline_layout_));

    updatePipeline(renderPass, sampleCount, blendDesc);
}

void VulkanPipeline::loadShader(std::string_view shaderFile)
{
    auto shaderModule = LoadShader(GetSpvShaderFile(shaderFile.data()), device_->getHandle());
    assert(shaderModule != VK_NULL_HANDLE);

    shader_modules_.push_back(shaderModule);

    auto shaderStage = pipelineShaderStageCreateInfo();
    shaderStage.stage = GetShaderType(shaderFile);
    shaderStage.module = shaderModule;
    shaderStage.pName = "main";

    shader_stages_.push_back(shaderStage);
}

void VulkanPipeline::updatePipeline(VkRenderPass renderPass, VkSampleCountFlagBits sampleCount, VkPipelineColorBlendStateCreateInfo* blendDesc)
{
    if (renderPass == VK_NULL_HANDLE) {
        return;
    }

    if (pipeline_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(device_->getHandle(), pipeline_, nullptr);
        pipeline_ = VK_NULL_HANDLE;
    }

    // 顶点信息输入的描述
    auto vertexInputState = pipelineVertexInputStateCreateInfo();
    vertexInputState.vertexBindingDescriptionCount = 0;
    vertexInputState.pVertexBindingDescriptions = nullptr; // Optional
    vertexInputState.vertexAttributeDescriptionCount = 0;
    vertexInputState.pVertexAttributeDescriptions = nullptr; // Optional

    // 输入装配阶段的信息，设置图元类型
    auto inputAssemblyState = pipelineInputAssemblyStateCreateInfo();
    inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyState.primitiveRestartEnable = VK_FALSE;

    // 光栅化阶段
    auto rasterizationState = pipelineRasterizationStateCreateInfo();
    rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationState.cullMode = VK_CULL_MODE_NONE;
    rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationState.rasterizerDiscardEnable = VK_FALSE;
    rasterizationState.depthBiasEnable = VK_FALSE;

    // 混合阶段的状态设置
    auto colorBlendState = pipelineColorBlendStateCreateInfo();
    if (blendDesc) {
        colorBlendState = *blendDesc;
    } else {
        auto blendAttachmentState = pipelineColorBlendAttachmentState(0xf, VK_FALSE);
        blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
        blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
        blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

        colorBlendState.attachmentCount = 1;
        colorBlendState.pAttachments = &blendAttachmentState;
        colorBlendState.logicOpEnable = VK_FALSE;
        colorBlendState.logicOp = VK_LOGIC_OP_NO_OP;
        colorBlendState.blendConstants[0] = 1.0f;
        colorBlendState.blendConstants[1] = 1.0f;
        colorBlendState.blendConstants[2] = 1.0f;
        colorBlendState.blendConstants[3] = 1.0f;
    }

    // 多重采样的设置
    auto multisampleState = pipelineMultisampleStateCreateInfo(sampleCount);

    // 视口状态设置
    auto viewportState = pipelineViewportStateCreateInfo(1, 1);
    viewportState.pViewports = nullptr;
    viewportState.pScissors = nullptr;

    // 深度、模板阶段的状态设置
    auto depthStencilState = pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE);
    depthStencilState.depthBoundsTestEnable = VK_FALSE;
    depthStencilState.stencilTestEnable = VK_FALSE;
    depthStencilState.back.failOp = VK_STENCIL_OP_KEEP;
    depthStencilState.back.passOp = VK_STENCIL_OP_KEEP;
    depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;
    depthStencilState.back.compareMask = 0;
    depthStencilState.back.reference = 0;
    depthStencilState.back.depthFailOp = VK_STENCIL_OP_KEEP;
    depthStencilState.back.writeMask = 0;
    depthStencilState.front = depthStencilState.back;
    depthStencilState.minDepthBounds = 0;
    depthStencilState.maxDepthBounds = 0;
    depthStencilState.stencilTestEnable = VK_FALSE;

    // 设置流水线的动态阶段
    std::vector<VkDynamicState> dynamicStateEnables = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_BLEND_CONSTANTS
    };
    auto dynamicState = pipelineDynamicStateCreateInfo();
    dynamicState.pDynamicStates = dynamicStateEnables.data();
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());


    // 创建流水线
    auto pipelineCI = pipelineCreateInfo();
    pipelineCI.stageCount = static_cast<uint32_t>(shader_stages_.size());
    pipelineCI.pStages = shader_stages_.data();
    pipelineCI.pVertexInputState = &vertexInputState;
    pipelineCI.pInputAssemblyState = &inputAssemblyState;
    pipelineCI.pRasterizationState = &rasterizationState;
    pipelineCI.pTessellationState = nullptr;
    pipelineCI.pColorBlendState = &colorBlendState;
    pipelineCI.pMultisampleState = &multisampleState;
    pipelineCI.pViewportState = &viewportState;
    pipelineCI.pDepthStencilState = &depthStencilState;
    pipelineCI.pDynamicState = &dynamicState;
    pipelineCI.layout = pipeline_layout_;
    pipelineCI.renderPass = renderPass;
    pipelineCI.subpass = 0;

    VK_CHECK(vkCreateGraphicsPipelines(device_->getHandle(), device_->getPipelineCache(), 1, &pipelineCI, nullptr, &pipeline_));
}

void VulkanPipeline::destroy()
{
    for (auto& module : shader_modules_) {
        vkDestroyShaderModule(device_->getHandle(), module, nullptr);
    }

    if (pipeline_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(device_->getHandle(), pipeline_, nullptr);
        pipeline_ = VK_NULL_HANDLE;
    }

    if (pipeline_layout_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device_->getHandle(), pipeline_layout_, nullptr);
        pipeline_layout_ = VK_NULL_HANDLE;
    }
}

void VulkanPipeline::draw(VkCommandBuffer cmdBuffer, VkDescriptorBufferInfo* pConstantBuffer, VkDescriptorSet descriptorSet)
{
    if (pipeline_ == VK_NULL_HANDLE) {
        LOG_WARN("Pipeline is not valid.");
        return;
    }

    // 设置绑定的常量缓冲区偏移
    int numUniformOffsets = 0;
    uint32_t uniformOffset = 0;
    if (pConstantBuffer != nullptr && pConstantBuffer->buffer != nullptr) {
        numUniformOffsets = 1;
        uniformOffset = static_cast<uint32_t>(pConstantBuffer->offset);
    }
    
    // 绑定描述符集
    if (descriptorSet != nullptr) {
        vkCmdBindDescriptorSets(cmdBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pipeline_layout_,
                                0,
                                1,
                                &descriptorSet,
                                numUniformOffsets,
                                &uniformOffset);
    }

    // 绑定流水线
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);

    // 绘制命令
    vkCmdDraw(cmdBuffer, 3, 1, 0, 0);
}

} // yu::vk
