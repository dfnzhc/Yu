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
#include "RHI/vulkan/appbase.hpp"
#include "RHI/vulkan/pipeline_builder.hpp"
#include "common/Bitmap.hpp"
#include "utils.hpp"
#include "RHI/vulkan/texture.hpp"
#include "RHI/vulkan/imgui.hpp"
#include "common/imgui_impl_glfw.h"
#include "RHI/vulkan/model_obj.hpp"

#include <glm/glm.hpp>
#include <imgui.h>
#include <imgui_internal.h>

using namespace yu;
using namespace yu::vk;

struct UIStates
{
    std::array<float, 50> frameTimes{};
    float frameTimeMin = 9999.0f, frameTimeMax = 0.0f;
} uiStates;

class RendererSample08 : public Renderer
{
public:
    void create(const VulkanDevice& device, SwapChain* swapChain, const MouseTracker& mouseTracker) override
    {
        Renderer::create(device, swapChain, mouseTracker);

        // 创建描述符布局（对着色器资源绑定的描述）
        std::vector<VkDescriptorSetLayoutBinding> layoutBinding(2);
        // 矩阵的描述
        layoutBinding[0] = yu::vk::descriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
            VK_SHADER_STAGE_VERTEX_BIT,
            0);

        // 采样器的描述
        layoutBinding[1] = yu::vk::descriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_SHADER_STAGE_FRAGMENT_BIT,
            1);

        // Uniform: 设置相机矩阵
        descriptor_pool_.createDescriptorSetLayoutAndAllocDescriptorSet(&layoutBinding,
                                                                        &descriptor_set_layout_,
                                                                        &descriptor_set_);
        constant_buffer_.setDescriptorSet(0, sizeof(glm::mat4) * 2, descriptor_set_);

        // Uniform: 图片
        // 加载图片
        texture_.createFromFile2D(device, upload_heap_, "texture.jpg");
        upload_heap_.flushAndFinish();

        // 创建图片视图
        texture_.createSRV(&texture_view_);

        // 创建图片采样器
        {
            auto info = samplerCreateInfo();
            info.magFilter = VK_FILTER_LINEAR;
            info.minFilter = VK_FILTER_LINEAR;
            info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

            info.anisotropyEnable = VK_TRUE;
            info.maxAnisotropy = device.getProperties().device_properties2.properties.limits.maxSamplerAnisotropy;
            info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            info.unnormalizedCoordinates = VK_FALSE;
            info.compareEnable = VK_FALSE;
            info.compareOp = VK_COMPARE_OP_ALWAYS;

            VK_CHECK(vkCreateSampler(device.getHandle(), &info, nullptr, &texture_sampler_));
        }

        // 设置图片的资源描述符信息
        {
            auto imageInfo = descriptorImageInfo(texture_sampler_,
                                                 texture_view_,
                                                 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

            auto write = writeDescriptorSet(descriptor_set_, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                            1, &imageInfo);
            vkUpdateDescriptorSets(device.getHandle(),
                                   1,
                                   &write,
                                   0,
                                   nullptr);
        }

        std::vector<VkVertexInputBindingDescription> bindingDesc;
        std::vector<VkVertexInputAttributeDescription> attrDesc;
        ModelObj::SetPipelineVertexInput(bindingDesc, attrDesc);

        pipeline_builder_.create(device);
        pipeline_builder_.setShader({"05_modelObj.vert", "05_modelObj.frag"});
        pipeline_builder_.setVertexInputState(bindingDesc, attrDesc);

        // 创建流水线
        pipeline_.create(device,
                         swapChain->getRenderPass(),
                         descriptor_set_layout_, pipeline_builder_);
        static_buffer_.uploadData(upload_heap_.getCommandBuffer());
        upload_heap_.flushAndFinish();
    }

    void destroy() override
    {
        // 释放流水线
        pipeline_.destroy();
        pipeline_builder_.destroy();

        // 释放描述符布局
        descriptor_pool_.freeDescriptor(descriptor_set_);
        vkDestroyDescriptorSetLayout(device_->getHandle(), descriptor_set_layout_, nullptr);

        texture_.destory();
        vkDestroySampler(device_->getHandle(), texture_sampler_, nullptr);
        vkDestroyImageView(device_->getHandle(), texture_view_, nullptr);

        Renderer::destroy();
    }

    void render() override
    {
        // 取得当前帧缓冲区的索引
        auto imageIndex = swap_chain_->waitForSwapChain();

        // 切换命令列表到当前帧
        constant_buffer_.beginFrame();
        frame_commands_.beginFrame();

        // 取到一个命令缓冲区，然后开始记录
        auto cmdBuffer = frame_commands_.getNewCommandBuffer();
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

            std::vector<VkClearValue> clearColor(2);
            clearColor[0].color = {0.1f, 0.2f, 0.23f, 1.0f};
            clearColor[1].depthStencil = {1.0f, 0};
            renderPassInfo.clearValueCount = static_cast<uint32_t>(clearColor.size());
            renderPassInfo.pClearValues = clearColor.data();

            vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        }

        // 动态更新视口和裁剪矩形
        vkCmdSetScissor(cmdBuffer, 0, 1, &rect_scissor_);
        vkCmdSetViewport(cmdBuffer, 0, 1, &viewport_);

        glm::mat4* mats;
        VkDescriptorBufferInfo constantBufferInfo;
        constant_buffer_.allocConstantBuffer(sizeof(glm::mat4) * 2, (void**) &mats, constantBufferInfo);
        mats[0] = mouse_tracker_->camera_->view_mat;
        mats[1] = mouse_tracker_->camera_->proj_mat;

        if (model_ != nullptr) {
            model_->draw(pipeline_, cmdBuffer, &constantBufferInfo, descriptor_set_);
        }

        imGui_->draw(cmdBuffer);

        // 停止 render pass 的记录
        vkCmdEndRenderPass(cmdBuffer);

        // 停止记录，并提交命令缓冲区
        {
            VK_CHECK(vkEndCommandBuffer(cmdBuffer));
            swap_chain_->submit(device_->getGraphicsQueue(), cmdBuffer);
        }

        // 交换链提交显示当前帧的命令，并转到下一帧
        VK_CHECK(swap_chain_->present());
    }

    int loadAssets(int loadingStage) override
    {
        const int stages = 12;
        // show loading progress
        ImGui::OpenPopup("Loading");
        if (ImGui::BeginPopupModal("Loading", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            float progress = static_cast<float>(loadingStage) / static_cast<float>(stages);
            ImGui::ProgressBar(progress, ImVec2(0.f, 0.f), nullptr);
            ImGui::EndPopup();
        }

        if (loadingStage == 0) {
        } else if (loadingStage == 5) {
            model_ = std::make_unique<yu::vk::ModelObj>();
            model_->load("cubebox_subdivided.obj", "cubebox/mesh/");
            model_->allocMemory(static_buffer_);
            static_buffer_.uploadData(upload_heap_.getCommandBuffer());
            upload_heap_.flushAndFinish();
        } else if (loadingStage == 9) {
            // flush 内存，释放暂存堆
            upload_heap_.flushAndFinish();
            static_buffer_.freeUploadHeap();

            return 0;
        }

        return loadingStage + 1;
    }

private:
    VulkanPipeline pipeline_;
    PipelineBuilder pipeline_builder_;

    std::unique_ptr<yu::vk::ModelObj> model_ = nullptr;

    Texture texture_;
    VkImageView texture_view_{};
    VkSampler texture_sampler_{};

    VkDescriptorBufferInfo vertex_buffer_info_{};
    VkDescriptorBufferInfo index_buffer_info_{};

    VkDescriptorSet descriptor_set_{};
    VkDescriptorSetLayout descriptor_set_layout_{};
};

class AppSample08 : public AppBase
{
public:
    AppSample08() : AppBase()
    {
        bSwapChain_CreateDepth = true;
    }

    ~AppSample08() override = default;

    void update(float delta_time) override
    {
        AppBase::update(delta_time);

        static float updateUITime = 0.0;
        updateUITime += delta_time;
        if (updateUITime > .5f) {
            std::rotate(uiStates.frameTimes.begin(), uiStates.frameTimes.begin() + 1, uiStates.frameTimes.end());
            uiStates.frameTimes.back() = fps_;
            if (fps_ < uiStates.frameTimeMin) {
                uiStates.frameTimeMin = fps_;
            }
            if (fps_ > uiStates.frameTimeMax) {
                uiStates.frameTimeMax = fps_;
            }
            updateUITime = 0.0f;
        }

        render();
    }

protected:
    void initRenderer() override
    {
        renderer_ = std::make_unique<RendererSample08>();
    }

    void render() override
    {
        buildGUI();
        renderer_->render();
    }

    void buildGUI()
    {
        ImGui::Begin("Debug");
        ImGui::Text("[FPS]: %d", static_cast<int>(uiStates.frameTimes.back()));

        ImGui::PlotLines("Frame Times", &uiStates.frameTimes[0], 50, 0, "", uiStates.frameTimeMin, uiStates.frameTimeMax, ImVec2(0, 80));
        ImGui::End();
    }

};

int main()
{
    San::LogSystem log;

    San::WinPlatform platform;
    platform.resize(1920, 1080);

    AppSample08 app{};
    platform.setApplication(&app);

    auto code = platform.initialize();
    if (code == San::ExitCode::Success) {

        code = platform.mainLoop();
    }

    platform.terminate(code);
}