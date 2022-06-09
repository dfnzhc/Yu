//
// Created by 秋鱼 on 2022/6/9.
//

#pragma once

#include "device.hpp"
#include "swap_chain.hpp"
#include "command.hpp"

namespace yu::vk {

class Renderer
{
public:
    Renderer() = default;
    ~Renderer() = default;

    virtual void create(VulkanDevice* device, SwapChain* swapChain);
    virtual void destroy();

    virtual void createWindowSizeDependency(uint32_t width, uint32_t height);
    virtual void destroyWindowSizeDependency();

    void resize(uint32_t width, uint32_t height);

    virtual void render();

protected:
    VulkanDevice* device_ = nullptr;
    SwapChain* swap_chain_ = nullptr;

    CommandList command_list_;

    VkRect2D rect_scissor_{};
    VkViewport viewport_{};

    uint32_t width_{};
    uint32_t height_{};

};

} // namespace yu::vk
