//
// Created by 秋鱼 on 2022/6/2.
//

#include "logger.hpp"
#include "RHI/vulkan/instance_properties.hpp"
#include "RHI/vulkan/instance.hpp"
#include "RHI/vulkan/device.hpp"
#include "win_platform.hpp"
#include "RHI/vulkan/swap_chain.hpp"
#include "RHI/vulkan/pipeline.hpp"
#include "RHI/vulkan/initializers.hpp"
#include "RHI/vulkan/error.hpp"
#include "RHI/vulkan/commands.hpp"

using namespace yu::vk;

int main()
{
    San::LogSystem log;

    San::WinPlatform platform;
    San::Application app{};
    platform.setApplication(&app);
    auto code = platform.initialize();

    {
        // 1. 创建 vulkan 实例
        VulkanInstance inst{"Test", InstanceProperties{}};
        inst.createSurface(platform.getWindow());

        // 2. 创建 vulkan 设备
        VulkanDevice device;
        device.create(inst);

        // 3. 创建交换链
        SwapChain swapChain{device};
        swapChain.createWindowSizeDependency(inst.getSurface());

        // 4. 创建流水线
        VulkanPipeline pipeline;
        PipelineBuilder pipelineBuilder;
        
        pipelineBuilder.create(device);
        pipelineBuilder.setShader({"01_shader_base.vert", "01_shader_base.frag"});

        // 4.1 创建描述符布局（对着色器资源绑定的描述）
        VkDescriptorSetLayout descriptorSetLayout{};
        auto layoutBinding = yu::vk::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0);
        auto descriptorLayout = yu::vk::descriptorSetLayoutCreateInfo(&layoutBinding, 1);
        VK_CHECK(vkCreateDescriptorSetLayout(device.getHandle(), &descriptorLayout, nullptr, &descriptorSetLayout));

        // 4.2 创建流水线
        pipeline.create(device, swapChain.getRenderPass(), descriptorSetLayout, pipelineBuilder);

        // 5.1 创建命令列表
        FrameCommands cmdList;
        const uint32_t commandBuffersPerFrame = 8;
        cmdList.create(device, SwapChain::FRAMES_IN_FLIGHT, commandBuffersPerFrame);

        // vulkan 实例与设备资源由析构函数释放
        while (!platform.getWindow()->shouldClose()) {
            try {
                platform.update();
                platform.getWindow()->processEvents();

                auto extent = platform.getWindow()->getExtent();
                const VkViewport viewport = yu::vk::viewport((float) extent.width, (float) extent.height, 0.0f, 1.0f);
                const VkRect2D scissor = yu::vk::rect2D((int32_t)extent.width, (int32_t)extent.height, 0, 0);

                // 取得当前帧缓冲区的索引
                auto imageIndex = swapChain.waitForSwapChain();

                // 切换命令列表到当前帧
                cmdList.beginFrame();

                // 取到一个命令缓冲区，然后开始记录
                auto cmdBuffer = cmdList.getNewCommandBuffer();
                {
                    auto cmd_buf_info = yu::vk::commandBufferBeginInfo();
                    cmd_buf_info.pNext = nullptr;
                    cmd_buf_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
                    cmd_buf_info.pInheritanceInfo = nullptr;
                    VK_CHECK(vkBeginCommandBuffer(cmdBuffer, &cmd_buf_info));
                }

                // render pass 一些默认设置
                {
                    auto renderPassInfo = renderPassBeginInfo();
                    renderPassInfo.renderPass = swapChain.getRenderPass();
                    renderPassInfo.framebuffer = swapChain.getFrameBuffer(static_cast<int>(imageIndex));
                    renderPassInfo.renderArea.offset = {0, 0};
                    renderPassInfo.renderArea.extent = {extent.width, extent.height};

                    VkClearValue clearColor = {{{0.2f, 0.3f, 0.7f, 1.0f}}};
                    renderPassInfo.clearValueCount = 1;
                    renderPassInfo.pClearValues = &clearColor;

                    vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
                }

                // 动态更新视口和裁剪矩形
                vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
                vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

                // 绘制设定的流水线
                pipeline.draw(cmdBuffer);

                // 停止 render pass 的记录
                vkCmdEndRenderPass(cmdBuffer);

                // 停止记录，并提交命令缓冲区
                {
                    VK_CHECK(vkEndCommandBuffer(cmdBuffer));

                    VkSemaphore ImageAvailableSemaphore;
                    VkSemaphore RenderFinishedSemaphores;
                    VkFence CmdBufExecutedFences;
                    swapChain.getSemaphores(&ImageAvailableSemaphore, &RenderFinishedSemaphores, &CmdBufExecutedFences);

                    VkPipelineStageFlags submitWaitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                    auto submit_info = submitInfo();
                    submit_info.pNext = nullptr;
                    submit_info.waitSemaphoreCount = 1;
                    submit_info.pWaitSemaphores = &ImageAvailableSemaphore;
                    submit_info.pWaitDstStageMask = &submitWaitStage;
                    submit_info.commandBufferCount = 1;
                    submit_info.pCommandBuffers = &cmdBuffer;
                    submit_info.signalSemaphoreCount = 1;
                    submit_info.pSignalSemaphores = &RenderFinishedSemaphores;

                    VK_CHECK(vkQueueSubmit(device.getGraphicsQueue(), 1, &submit_info, CmdBufExecutedFences));

                }

                // 交换链提交显示当前帧的命令，并转到下一帧
                VK_CHECK(swapChain.present());
            }
            catch (std::exception& e) {
                LOG_ERROR("Error Message: {}", e.what());
            }
        }

        vkDeviceWaitIdle(device.getHandle());
        
        // 5.2 释放命令池与命令缓冲区
        cmdList.destroy();

        // 4.3 释放流水线
        pipeline.destroy();
        pipelineBuilder.destroy();

        // 4.4 释放描述符布局
        vkDestroyDescriptorSetLayout(device.getHandle(), descriptorSetLayout, nullptr);

        // 3.1 手动释放交换链中依赖窗口的部件
        swapChain.destroyWindowSizeDependency();

        auto [d_name, d_api] = device.getProperties().getDeviceInfo();

        LOG_INFO("Found device: {}. {}", d_name, d_api);

    }

//    if (code == San::ExitCode::Success) {
//        
//        code = platform.mainLoop();
//    }

    platform.terminate(code);
}