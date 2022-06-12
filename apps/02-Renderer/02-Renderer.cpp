﻿//
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
#include "RHI/vulkan/command.hpp"
#include "RHI/vulkan/appbase.hpp"

using namespace yu::vk;

class RendererSample02 : public Renderer
{
public:
    void create(VulkanDevice* device, SwapChain* swapChain) override
    {
        Renderer::create(device, swapChain);

        // 创建描述符布局（对着色器资源绑定的描述）
        auto layoutBinding = yu::vk::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0);
        auto descriptorLayout = yu::vk::descriptorSetLayoutCreateInfo(&layoutBinding, 1);
        VK_CHECK(vkCreateDescriptorSetLayout(device->getHandle(), &descriptorLayout, nullptr, &descriptor_set_layout_));

        // 创建流水线
        pipeline_.create(*device, swapChain->getRenderPass(), {"01_shader_base.vert", "01_shader_base.frag"}, descriptor_set_layout_);
    }

    void destroy() override
    {
        Renderer::destroy();

        // 释放流水线
        pipeline_.destroy();

        // 释放描述符布局
        vkDestroyDescriptorSetLayout(device_->getHandle(), descriptor_set_layout_, nullptr);
    }

    void render() override
    {
        // 取得当前帧缓冲区的索引
        auto imageIndex = swap_chain_->waitForSwapChain();

        // 切换命令列表到当前帧
        command_list_.beginFrame();

        // 取到一个命令缓冲区，然后开始记录
        auto cmdBuffer = command_list_.getNewCommandBuffer();
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
            renderPassInfo.renderPass = swap_chain_->getRenderPass();
            renderPassInfo.framebuffer = swap_chain_->getFrameBuffer(static_cast<int>(imageIndex));
            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent = {width_, height_};

            VkClearValue clearColor = {{{0.2f, 0.3f, 0.7f, 1.0f}}};
            renderPassInfo.clearValueCount = 1;
            renderPassInfo.pClearValues = &clearColor;

            vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        }

        // 动态更新视口和裁剪矩形
        vkCmdSetScissor(cmdBuffer, 0, 1, &rect_scissor_);
        vkCmdSetViewport(cmdBuffer, 0, 1, &viewport_);

        // 绘制设定的流水线
        pipeline_.draw(cmdBuffer);

        // 停止 render pass 的记录
        vkCmdEndRenderPass(cmdBuffer);

        // 停止记录，并提交命令缓冲区
        {
            VK_CHECK(vkEndCommandBuffer(cmdBuffer));

            VkSemaphore ImageAvailableSemaphore;
            VkSemaphore RenderFinishedSemaphores;
            VkFence CmdBufExecutedFences;
            swap_chain_->getSemaphores(&ImageAvailableSemaphore, &RenderFinishedSemaphores, &CmdBufExecutedFences);

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

            VK_CHECK(vkQueueSubmit(device_->getGraphicsQueue(), 1, &submit_info, CmdBufExecutedFences));
        }
        
        Renderer::render();
    }

private:
    VulkanPipeline pipeline_;

    VkDescriptorSetLayout descriptor_set_layout_{};
};

class AppSample02 : public AppBase
{
public:
    ~AppSample02() override = default;

    void update(float delta_time) override
    {
        AppBase::update(delta_time);
        
        render();
    }

protected:
    void initRenderer() override
    {
        renderer_ = std::make_unique<RendererSample02>();
    }

    void render() override
    {
        renderer_->render();
    }
};

int main()
{
    San::LogSystem log;

    San::WinPlatform platform;

    AppSample02 app{};
    platform.setApplication(&app);

    auto code = platform.initialize();
    if (code == San::ExitCode::Success) {

        code = platform.mainLoop();
    }

    platform.terminate(code);
}