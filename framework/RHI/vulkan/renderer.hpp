//
// Created by 秋鱼 on 2022/6/9.
//

#pragma once

#include "device.hpp"
#include "swap_chain.hpp"
#include "commands.hpp"
#include "dynamic_buffer.hpp"
#include "descriptor_pool.hpp"
#include "static_buffer.hpp"
#include "async.hpp"
#include "upload_heap.hpp"
#include "imgui.hpp"
#include "gpu_time.hpp"

#include <common/mouse_tracker.hpp>

namespace yu::vk {

class Renderer
{
public:
    Renderer() = default;
    ~Renderer() = default;

    virtual void create(const VulkanDevice& device, SwapChain* swapChain, const MouseTracker& mouseTracker);
    virtual void destroy();

    virtual void createWindowSizeDependency(uint32_t width, uint32_t height);
    virtual void destroyWindowSizeDependency();

    void resize(uint32_t width, uint32_t height);

    virtual void render();
    virtual int loadAssets(int loadingStage);
    
    void createUI(const VulkanInstance& instance, GLFWwindow* window);
    
    const std::vector<TimeStamp>& getTimings() const { return time_stamps_; }
    
protected:
    const VulkanDevice* device_ = nullptr;
    SwapChain* swap_chain_ = nullptr;
    const MouseTracker* mouse_tracker_ = nullptr;

    FrameCommands frame_commands_;
    DynamicBuffer constant_buffer_;
    DescriptorPool descriptor_pool_;
    StaticBuffer static_buffer_;
    UploadHeap upload_heap_;

    VkRect2D rect_scissor_{};
    VkViewport viewport_{};

    uint32_t width_{};
    uint32_t height_{};
    
    GPUTimeStamp gpu_timer_{}; 
    std::vector<TimeStamp> time_stamps_;
    
    std::unique_ptr<ImGUI> imGui_ = nullptr;
    San::AsyncPool async_pool_;
};

} // namespace yu::vk
