﻿//
// Created by 秋鱼 on 2022/6/9.
//

#include "renderer.hpp"
#include "initializers.hpp"
#include "error.hpp"

namespace yu::vk {

void Renderer::create(const VulkanDevice& device, SwapChain* swapChain, const MouseTracker& mouseTracker)
{
    device_ = &device;
    swap_chain_ = swapChain;
    mouse_tracker_ = &mouseTracker;

    // 创建命令列表
    const uint32_t commandBuffersPerFrame = 8;
    command_list_.create(device, swapChain->getFrameCount(), commandBuffersPerFrame);

    // 创建动态的常量缓冲区，用于设定 uniform、索引、顶点数据
    const uint32_t constantBuffersMemSize = 200 * 1024 * 1024;
    constant_buffer_.create(device, swapChain->getFrameCount(), constantBuffersMemSize, "Uniforms");

    // 创建描述符堆，用来创建相应的描述符
    const uint32_t cbvDescriptorCount = 2000;
    const uint32_t srvDescriptorCount = 8000;
    const uint32_t samplerDescriptorCount = 20;
    const uint32_t uavDescriptorCount = 10;
    descriptor_pool_.create(device, cbvDescriptorCount, srvDescriptorCount, samplerDescriptorCount, uavDescriptorCount);

    // 创建一个顶点缓冲区，用于上传顶点、索引数据
    const uint32_t vertexMemSize = (1 * 128) * 1024 * 1024;
    vertex_buffer_.create(device, vertexMemSize, false, "VertexData");
}

void Renderer::destroy()
{
    command_list_.destroy();
    constant_buffer_.destroy();
    descriptor_pool_.destroy();
    vertex_buffer_.destroy();
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