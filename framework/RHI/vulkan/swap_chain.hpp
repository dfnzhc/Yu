//
// Created by 秋鱼 on 2022/6/4.
//

#pragma once

#include "device.hpp"

namespace yu::vk {

class SwapChain
{
public:
    SwapChain(const VulkanDevice& device);
    ~SwapChain();
    
    void createWindowSizeDependency(VkSurfaceKHR surface, bool VSync = false);
    void destroyWindowSizeDependency();
    
private:
    void getSurfaceFormat();
    void createImageAndRTV();
    void createFrameBuffers(uint32_t width, uint32_t height);
    
private:
    VkSwapchainKHR swap_chain_{};
    
    VkSurfaceKHR surface_;
    const VulkanDevice* device_ = nullptr;
    
    VkQueue present_queue_{};
    
    VkFormat color_format_;
    VkColorSpaceKHR color_space_;
    
    uint32_t image_count_{};
    std::vector<VkImage> images_;
    std::vector<VkImageView> image_views_;
    std::vector<VkFramebuffer> frame_buffers_;
};

} // yu::vk