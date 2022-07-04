//
// Created by 秋鱼 on 2022/6/8.
//

#pragma once

#include "device.hpp"
namespace yu::vk {

/**
 * @brief 用于创建命令池和命令缓冲区
 */
class FrameCommands
{
public:
    void create(const VulkanDevice& device, uint32_t numberOfFrames, uint32_t commandBufferPerFrame, bool isCompute = false);
    void destroy();
    
    void beginFrame();
    VkCommandBuffer getNewCommandBuffer();

private:
    const VulkanDevice* device_ = nullptr;

    uint32_t frame_index_{};
    uint32_t number_of_frames_{};
    uint32_t command_buffer_per_frame_{};

    struct CommandBufferPerFrame
    {
        VkCommandPool command_pool{};
        std::vector<VkCommandBuffer> command_buffer{};
        uint32_t number_of_used{};
    };
    
    std::vector<CommandBufferPerFrame> command_buffers{};
    CommandBufferPerFrame* current_buffer = nullptr;
};

} // namespace yu::vk