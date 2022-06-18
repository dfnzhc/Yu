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
                            VkDescriptorSetLayout descriptorSetLayout,
                            PipelineBuilder& pipelineBuilder)
{
    device_ = &device;

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

    pipelineBuilder.createPipeline(renderPass, pipeline_layout_, pipeline_);
}

void VulkanPipeline::destroy()
{
    if (pipeline_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(device_->getHandle(), pipeline_, nullptr);
        pipeline_ = VK_NULL_HANDLE;
    }

    if (pipeline_layout_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device_->getHandle(), pipeline_layout_, nullptr);
        pipeline_layout_ = VK_NULL_HANDLE;
    }
}

void VulkanPipeline::draw(VkCommandBuffer cmdBuffer,
                          VkDescriptorBufferInfo* pConstantBuffer,
                          VkDescriptorSet descriptorSet)
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

void VulkanPipeline::draw(VkCommandBuffer cmdBuffer,
                          uint32_t vertexCount,
                          VkDescriptorBufferInfo* pVertexBuffer,
                          VkDescriptorBufferInfo* pConstantBuffer,
                          VkDescriptorSet descriptorSet)
{
    if (pipeline_ == VK_NULL_HANDLE) {
        LOG_WARN("Pipeline is not valid.");
        return;
    }
    
    if (!pVertexBuffer) {
        LOG_ERROR("Vertex buffer is invalid.")
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
    
    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &pVertexBuffer->buffer, &pVertexBuffer->offset);

    // 绑定流水线
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);

    // 绘制命令
    vkCmdDraw(cmdBuffer, vertexCount, 1, 0, 0);
}

} // yu::vk
