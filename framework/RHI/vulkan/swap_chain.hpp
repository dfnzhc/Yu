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

    VkImage getCurrentBackBuffer() { return images_[image_index_]; }
    VkImageView getCurrentBackBufferRTV() { return image_views_[image_index_]; }
    VkFramebuffer getCurrentFrameBuffer() const { return frame_buffers_[image_index_]; }

    VkSwapchainKHR getHandle() const { return swap_chain_; }
    VkFormat getFormat() const { return format_; }
    VkRenderPass getRenderPass() const { return render_pass_; }
    VkFramebuffer getFrameBuffer(int i) const { return frame_buffers_[i]; }

    uint32_t waitForSwapChain();
    void getSemaphores(VkSemaphore *pImageAvailableSemaphore, VkSemaphore *pRenderFinishedSemaphores, VkFence *pCmdBufExecutedFences);
    VkResult present();
    
    static constexpr uint32_t FRAMES_IN_FLIGHT = 2;

private:
    void getSurfaceFormat();
    void createImageAndRTV();

    void createRenderPass();
    void destroyRenderPass();

    void createFrameBuffers(uint32_t width, uint32_t height);
    void destroyFrameBuffers();
private:
    VkSwapchainKHR swap_chain_{};

    VkSurfaceKHR surface_;
    const VulkanDevice* device_ = nullptr;

    VkQueue present_queue_{};

    VkRenderPass render_pass_{};

    VkFormat format_;
    VkColorSpaceKHR color_space_;

    uint32_t image_count_{};
    uint32_t image_index_ = 0;
    std::vector<VkImage> images_;
    std::vector<VkImageView> image_views_;
    std::vector<VkFramebuffer> frame_buffers_;
   
    uint32_t current_frame_ = 0;
    uint32_t prev_frame_ = 0;
    
    std::vector<VkFence> cmdBuf_executed_fences_;
    std::vector<VkSemaphore> image_available_semaphores_;
    std::vector<VkSemaphore> render_finished_semaphores_;
};

} // yu::vk