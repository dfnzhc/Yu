//
// Created by 秋鱼 on 2022/6/8.
//

#include "commands.hpp"
#include "initializers.hpp"
#include "error.hpp"

namespace yu::vk {

void FrameCommands::create(const VulkanDevice& device, uint32_t numberOfFrames, uint32_t commandBufferPerFrame, bool isCompute)
{
    device_ = &device;
    number_of_frames_ = numberOfFrames;
    command_buffer_per_frame_ = commandBufferPerFrame;

    // 为每一帧创建独立命令池，让命令池能够各自分配命令缓冲区
    command_buffers.resize(number_of_frames_);

    for (auto& buf : command_buffers) {
        // 创建命令池
        auto cmdPoolInfo = commandPoolCreateInfo();
        cmdPoolInfo.pNext = nullptr;
        if (isCompute) {
            cmdPoolInfo.queueFamilyIndex = device_->getComputeQueueIndex();
        } else {
            cmdPoolInfo.queueFamilyIndex = device_->getGraphicsQueueIndex();
        }

        cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

        VK_CHECK(vkCreateCommandPool(device_->getHandle(), &cmdPoolInfo, nullptr, &buf.command_pool));

        // 创建命令缓冲区
        buf.command_buffer.resize(command_buffer_per_frame_);
        auto cmdBufferInfo = commandBufferAllocateInfo(buf.command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, command_buffer_per_frame_);

        VK_CHECK(vkAllocateCommandBuffers(device_->getHandle(), &cmdBufferInfo, buf.command_buffer.data()));

        buf.number_of_used = 0;
    }

    frame_index_ = 0;
}

void FrameCommands::destroy()
{
    for (auto& buf : command_buffers) {
        vkFreeCommandBuffers(device_->getHandle(), buf.command_pool, command_buffer_per_frame_, buf.command_buffer.data());
        vkDestroyCommandPool(device_->getHandle(), buf.command_pool, nullptr);
    }
}

void FrameCommands::beginFrame()
{
    current_buffer = &command_buffers[frame_index_ % number_of_frames_];
    current_buffer->number_of_used = 0;
    
    frame_index_ += 1;
}

VkCommandBuffer FrameCommands::getNewCommandBuffer()
{
    auto cmdBuffer = current_buffer->command_buffer[current_buffer->number_of_used++];

    assert(current_buffer->number_of_used < command_buffer_per_frame_);
    
    return cmdBuffer;
}

} // namespace yu::vk
