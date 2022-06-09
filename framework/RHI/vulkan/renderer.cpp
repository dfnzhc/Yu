//
// Created by 秋鱼 on 2022/6/9.
//

#include "renderer.hpp"
#include "initializers.hpp"
#include "error.hpp"

namespace yu::vk {

void Renderer::create(VulkanDevice* device, SwapChain* swapChain)
{
    device_ = device;
    swap_chain_ = swapChain;

    // 创建命令列表
    const uint32_t commandBuffersPerFrame = 8;
    command_list_.create(*device, SwapChain::FRAMES_IN_FLIGHT, swapChain->getFrameCount());
}

void Renderer::destroy()
{
    command_list_.destroy();
}

void Renderer::createWindowSizeDependency(uint32_t width, uint32_t height)
{
    width_ = width;
    height_ = height;

    rect_scissor_ = rect2D(width_, height_, 0, 0);

    viewport_ = viewport(static_cast<float>(width_), static_cast<float>(height_), 0.0f, 1.0f);
}

void Renderer::destroyWindowSizeDependency()
{

}

void Renderer::resize(uint32_t width, uint32_t height)
{
    destroyWindowSizeDependency();

    createWindowSizeDependency(width, height);
}

void Renderer::render()
{
    // 交换链提交显示当前帧的命令，并转到下一帧
    VK_CHECK(swap_chain_->present());
}

} // namespace yu::vk