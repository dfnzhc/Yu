//
// Created by 秋鱼 on 2022/6/25.
//

#pragma once

#include <GLFW/glfw3.h>
#include "device.hpp"
#include "dynamic_buffer.hpp"
#include "upload_heap.hpp"
#include "pipeline_builder.hpp"

#include <glm/vec2.hpp>

namespace yu::vk {

constexpr uint32_t DESCRIPTOR_COUNT = 128;

class ImGUI
{
public:
    ImGUI();
    ~ImGUI();
    void create(const VulkanDevice& device,
                const DynamicBuffer& constantBuffer,
                VkRenderPass renderPass,
                UploadHeap& uploadHeap);
    void destroy();

    void updatePipeline(VkRenderPass renderPass);
    void draw(VkCommandBuffer cmdBuffer);

private:
    const VulkanDevice* device_ = nullptr;
    DynamicBuffer* constant_buffer_ = nullptr;

    VkImage font_texture_{};
    VkImageView texture_view_{};
    VkSampler sampler_{};
    VmaAllocation tex_allocation_{};
    VkDeviceMemory font_memory_{};

    std::vector<VkDescriptorSet> descriptor_set_;
    VkDescriptorPool descriptor_pool_{};
    VkDescriptorSetLayout descriptor_set_layout_{};
    VkDescriptorBufferInfo descriptor_buffer_info_{};
    uint32_t current_descriptor_index_ = 0;

    VkPipeline pipeline_{};
    VkPipelineLayout pipeline_layout_{};
    PipelineBuilder pipeline_builder_;

    struct PushConstBlock
    {
        glm::vec2 scale{};
        glm::vec2 translate{};
    } push_const_block_;
};

} // yu::vk