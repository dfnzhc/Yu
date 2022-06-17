//
// Created by 秋鱼 on 2022/6/9.
//

#pragma once

#include "device.hpp"
#include "swap_chain.hpp"
#include "command.hpp"
#include "dynamic_buffer.hpp"
#include "descriptor_heap.hpp"
#include "static_buffer.hpp"

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

protected:
    const VulkanDevice* device_ = nullptr;
    SwapChain* swap_chain_ = nullptr;
    const MouseTracker* mouse_tracker_ = nullptr;

    CommandList command_list_;
    DynamicBuffer constant_buffer_;
    DescriptorHeap descriptor_heap_;
    StaticBuffer vertex_buffer_;

    VkRect2D rect_scissor_{};
    VkViewport viewport_{};

    uint32_t width_{};
    uint32_t height_{};

};

} // namespace yu::vk
