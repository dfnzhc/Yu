//
// Created by 秋鱼 on 2022/6/4.
//

#include <logger.hpp>
#include "swap_chain.hpp"
#include "error.hpp"
#include "initializers.hpp"

namespace yu::vk {

SwapChain::SwapChain(const VulkanDevice& device) : device_{&device}
{
    present_queue_ = device_->getPresentQueue();

    // 设置默认的交换链格式
    format_ = VK_FORMAT_R8G8B8A8_UNORM;
    color_space_ = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

    // 创建前后缓冲区之间的同步原语
    cmdBuf_executed_fences_.resize(FRAMES_IN_FLIGHT);
    image_available_semaphores_.resize(FRAMES_IN_FLIGHT);
    render_finished_semaphores_.resize(FRAMES_IN_FLIGHT);

    auto fenceInfo = fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    auto semaphoreInfo = semaphoreCreateInfo();
    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
        VK_CHECK(vkCreateFence(device_->getHandle(), &fenceInfo, nullptr, &cmdBuf_executed_fences_[i]));
        VK_CHECK(vkCreateSemaphore(device_->getHandle(), &semaphoreInfo, nullptr, &image_available_semaphores_[i]));
        VK_CHECK(vkCreateSemaphore(device_->getHandle(), &semaphoreInfo, nullptr, &render_finished_semaphores_[i]));
    }

    createRenderPass();
}

SwapChain::~SwapChain()
{
    destroyRenderPass();

    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
        vkDestroyFence(device_->getHandle(), cmdBuf_executed_fences_[i], nullptr);
        vkDestroySemaphore(device_->getHandle(), image_available_semaphores_[i], nullptr);
        vkDestroySemaphore(device_->getHandle(), render_finished_semaphores_[i], nullptr);
    }
}

/**
 * @brief 创建依赖于窗口的交换链属性
 * 
 * @throw runtime_error 如果 surface 是无效的
 */
void SwapChain::createWindowSizeDependency(VkSurfaceKHR surface, bool VSync)
{
    surface_ = surface;
    VkDevice device = device_->getHandle();
    VkPhysicalDevice physicalDevice = device_->getProperties().physical_device;

    // 获取 surface 的格式
    getSurfaceFormat();

    // 按照 surface 格式，创建 render pass
    destroyRenderPass();
    createRenderPass();

    VkSurfaceCapabilitiesKHR surfCapabilities;
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface_, &surfCapabilities));

    VkExtent2D swapchainExtent;
    if (surfCapabilities.currentExtent.width != 0xFFFFFFFF) {
        swapchainExtent.width = surfCapabilities.currentExtent.width;
        swapchainExtent.height = surfCapabilities.currentExtent.height;
    } else {
        LOG_FATAL("Surface size if undefined, check the validation of the surface.");
    }

    // 确定支持的图像数量
    uint32_t desiredNumberOfSwapchainImages = surfCapabilities.minImageCount + 1;
    if ((surfCapabilities.maxImageCount > 0) && (desiredNumberOfSwapchainImages > surfCapabilities.maxImageCount)) {
        desiredNumberOfSwapchainImages = surfCapabilities.maxImageCount;
    }

    image_count_ = desiredNumberOfSwapchainImages;

    // 找出 surface 的变换
    VkSurfaceTransformFlagsKHR preTransform;
    if (surfCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        // 更倾向于采用非旋转的变换
        preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    } else {
        preTransform = surfCapabilities.currentTransform;
    }

    // 找到一个支持的组合透明度的格式（不是所有设备都支持 alpha 不透明）
    VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    // 选择第一个可用的组合透明度格式
    std::vector<VkCompositeAlphaFlagBitsKHR> compositeAlphaFlags = {
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
    };
    for (auto& compositeAlphaFlag : compositeAlphaFlags) {
        if (surfCapabilities.supportedCompositeAlpha & compositeAlphaFlag) {
            compositeAlpha = compositeAlphaFlag;
            break;
        }
    }
    // 选择交换链的显示模式

    // 获取可用的显示模式
    uint32_t presentModeCount;
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface_, &presentModeCount, nullptr));
    assert(presentModeCount > 0);

    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface_, &presentModeCount, presentModes.data()));
    // 默认使用 VK_PRESENT_MODE_FIFO_KHR 模式
    // 这种模式会等待垂直同步（"v-sync"）
    VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;

    // 如果不要求 v-sync，则尝试设置为邮箱模式
    // 这是可用的延迟最低的非撕裂式存在模式
    if (!VSync) {
        for (size_t i = 0; i < presentModeCount; i++) {
            if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
                swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            }
            if (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
                swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            }
        }
    }

    // 存储当前的交换链句柄，以便之后用它来进行重建交换链
    VkSwapchainKHR oldSwapchain = swap_chain_;

    auto swapchain_info = swapChainCreateInfo();
    swapchain_info.surface = surface;
    swapchain_info.minImageCount = image_count_;
    swapchain_info.imageFormat = format_;
    swapchain_info.imageColorSpace = color_space_;
    swapchain_info.imageExtent = {swapchainExtent.width, swapchainExtent.height};
    swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_info.preTransform = (VkSurfaceTransformFlagBitsKHR) preTransform;
    swapchain_info.imageArrayLayers = 1;
    swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_info.queueFamilyIndexCount = 0;
    swapchain_info.presentMode = swapchainPresentMode;
    // 将oldSwapChain设置为前一个交换链的保存柄，有助于资源的再利用，并确保我们仍然可以展示已经获得的图像
    swapchain_info.oldSwapchain = oldSwapchain;
    // 将clipped设置为VK_TRUE，允许实现放弃 surface 区域以外的渲染
    swapchain_info.clipped = VK_TRUE;
    swapchain_info.compositeAlpha = compositeAlpha;

    // 如果支持，在交换链图像上启用传输源
    if (surfCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
        swapchain_info.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }

    // 如果支持的话，在交换链图像上启用传输目标
    if (surfCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
        swapchain_info.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }

    VK_CHECK(vkCreateSwapchainKHR(device, &swapchain_info, nullptr, &swap_chain_));

    // 如果一个现有的交换链被重新创建，那么需要销毁旧的交换链
    // 这也会对所有图像进行清理
    if (oldSwapchain != VK_NULL_HANDLE) {
        for (uint32_t i = 0; i < image_count_; i++) {
            vkDestroyImageView(device, image_views_[i], nullptr);
        }
        vkDestroySwapchainKHR(device, oldSwapchain, nullptr);
    }

    // 创建对应数量的图像和图像视图
    VK_CHECK(vkGetSwapchainImagesKHR(device, swap_chain_, &image_count_, nullptr));

    createImageAndRTV();
    createFrameBuffers(swapchainExtent.width, swapchainExtent.height);

    image_index_ = 0;
}

void SwapChain::destroyWindowSizeDependency()
{
    destroyRenderPass();
    destroyFrameBuffers();

    // 摧毁图像视图
    for (auto& image_view : image_views_) {
        vkDestroyImageView(device_->getHandle(), image_view, nullptr);
    }

    // 摧毁交换链
    if (swap_chain_ != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device_->getHandle(), swap_chain_, nullptr);
        swap_chain_ = VK_NULL_HANDLE;
    }
}

void SwapChain::getSurfaceFormat()
{
    // 获取 surface 的表面格式
    uint32_t formatCount;
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(device_->getProperties().physical_device, surface_, &formatCount, nullptr));
    assert(formatCount > 0);

    std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(device_->getProperties().physical_device, surface_, &formatCount, surfaceFormats.data()));

    // 如果查询的 surface 格式只有 VK_FORMAT_UNDEFINED,
    // 那就没有首选的格式，假定使用 VK_FORMAT_B8G8R8A8_UNORM
    if ((formatCount == 1) && (surfaceFormats[0].format == VK_FORMAT_UNDEFINED)) {
        format_ = VK_FORMAT_B8G8R8A8_UNORM;
        color_space_ = surfaceFormats[0].colorSpace;
    } else {
        // 遍历可用的 surface 格式列表，并且检查是否存在 VK_FORMAT_B8G8R8A8_UNORM
        bool found_B8G8R8A8_UNORM = false;
        for (auto&& surfaceFormat : surfaceFormats) {
            if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM) {
                format_ = surfaceFormat.format;
                color_space_ = surfaceFormat.colorSpace;
                found_B8G8R8A8_UNORM = true;
                break;
            }
        }

        // 当 VK_FORMAT_B8G8R8A8_UNORM 不可用时，选择第一个可用的颜色格式
        if (!found_B8G8R8A8_UNORM) {
            format_ = surfaceFormats[0].format;
            color_space_ = surfaceFormats[0].colorSpace;
        }
    }
}

void SwapChain::createImageAndRTV()
{
    // 取得交换链上的图像
    images_.resize(image_count_);
    VK_CHECK(vkGetSwapchainImagesKHR(device_->getHandle(), swap_chain_, &image_count_, images_.data()));

    // 重新创建图像视图，也就是 Render Target View(RTV)
    image_views_.resize(image_count_);
    for (uint32_t i = 0; i < image_count_; i++) {
        VkImageViewCreateInfo colorAttachmentView = {};
        colorAttachmentView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        colorAttachmentView.pNext = nullptr;
        colorAttachmentView.format = format_;
        colorAttachmentView.components = {
            VK_COMPONENT_SWIZZLE_R,
            VK_COMPONENT_SWIZZLE_G,
            VK_COMPONENT_SWIZZLE_B,
            VK_COMPONENT_SWIZZLE_A
        };
        colorAttachmentView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        colorAttachmentView.subresourceRange.baseMipLevel = 0;
        colorAttachmentView.subresourceRange.levelCount = 1;
        colorAttachmentView.subresourceRange.baseArrayLayer = 0;
        colorAttachmentView.subresourceRange.layerCount = 1;
        colorAttachmentView.viewType = VK_IMAGE_VIEW_TYPE_2D;
        colorAttachmentView.flags = 0;

        colorAttachmentView.image = images_[i];

        VK_CHECK(vkCreateImageView(device_->getHandle(), &colorAttachmentView, nullptr, &image_views_[i]));
    }
}

void SwapChain::createRenderPass()
{
    VkSurfaceFormatKHR surfaceFormat;
    surfaceFormat.format = VK_FORMAT_R8G8B8A8_UNORM;
    surfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

    // color attachment
    VkAttachmentDescription attachments[1];
    attachments[0].format = format_;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    attachments[0].flags = 0;

    VkAttachmentReference colorReference = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    VkSubpassDescription subpassDescription = {};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colorReference;
    subpassDescription.pDepthStencilAttachment = nullptr;
    subpassDescription.inputAttachmentCount = 0;
    subpassDescription.pInputAttachments = nullptr;
    subpassDescription.preserveAttachmentCount = 0;
    subpassDescription.pPreserveAttachments = nullptr;
    subpassDescription.pResolveAttachments = nullptr;

    // Subpass dependencies for layout transitions
    std::array<VkSubpassDependency, 1> dependencies{};

    dependencies[0].dependencyFlags = 0;
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = 0;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    auto renderPass_info = renderPassCreateInfo();
    renderPass_info.pNext = nullptr;
    renderPass_info.attachmentCount = 1;
    renderPass_info.pAttachments = attachments;
    renderPass_info.subpassCount = 1;
    renderPass_info.pSubpasses = &subpassDescription;
    renderPass_info.dependencyCount = 1;
    renderPass_info.pDependencies = dependencies.data();

    VK_CHECK(vkCreateRenderPass(device_->getHandle(), &renderPass_info, nullptr, &render_pass_));
}

void SwapChain::destroyRenderPass()
{
    if (render_pass_ != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device_->getHandle(), render_pass_, nullptr);
        render_pass_ = VK_NULL_HANDLE;
    }
}

void SwapChain::createFrameBuffers(uint32_t width, uint32_t height)
{
    frame_buffers_.resize(image_count_);
    for (uint32_t i = 0; i < image_count_; i++) {
        VkImageView attachments[] = {image_views_[i]};

        VkFramebufferCreateInfo fb_info = {};
        fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fb_info.pNext = nullptr;
        fb_info.renderPass = render_pass_;
        fb_info.attachmentCount = 1;
        fb_info.pAttachments = attachments;
        fb_info.width = width;
        fb_info.height = height;
        fb_info.layers = 1;

        VK_CHECK(vkCreateFramebuffer(device_->getHandle(), &fb_info, nullptr, &frame_buffers_[i]));
    }
}

void SwapChain::destroyFrameBuffers()
{
    for (auto& fmBuffer : frame_buffers_) {
        vkDestroyFramebuffer(device_->getHandle(), fmBuffer, nullptr);
    }
}

/**
 * @brief 等待当前帧的命令缓冲区执行完命令，然后获取交换链中下一个可用的图像（缓冲区）索引
 */
uint32_t SwapChain::waitForSwapChain()
{
    vkWaitForFences(device_->getHandle(), 1, &cmdBuf_executed_fences_[current_frame_], VK_TRUE, UINT64_MAX);

    VK_CHECK(vkAcquireNextImageKHR(device_->getHandle(),
                                   swap_chain_,
                                   UINT64_MAX,
                                   image_available_semaphores_[current_frame_],
                                   VK_NULL_HANDLE,
                                   &image_index_));

    vkResetFences(device_->getHandle(), 1, &cmdBuf_executed_fences_[current_frame_]);

    return image_index_;
}

/**
 * @brief 取得用于渲染当前帧的同步原语
 * 
 * @param pImageAvailableSemaphore: 指示交换链中图像已可用的同步信号，表示渲染之前应该等待的信号
 * @param pRenderFinishedSemaphores: 指示当前帧渲染完毕后发出的信号
 * @param pCmdBufExecutedFences: 指示当前帧的命令缓冲区的同步栅栏，用于 CPU 与 GPU 之间的同步
 */
void SwapChain::getSemaphores(VkSemaphore* pImageAvailableSemaphore, VkSemaphore* pRenderFinishedSemaphores, VkFence* pCmdBufExecutedFences)
{
    *pImageAvailableSemaphore = image_available_semaphores_[current_frame_];
    *pRenderFinishedSemaphores = render_finished_semaphores_[current_frame_];
    *pCmdBufExecutedFences = cmdBuf_executed_fences_[current_frame_];
}

/**
 * @brief 向 present 队列提交呈现当帧到屏幕的命令，等待原语的意义是：等到当前帧完成了渲染，那么就把它呈现到屏幕上
 * @return 提交的结果，把提交到队列的结果返回，让调用者决定之后的操作
 */
VkResult SwapChain::present()
{
    auto present = presentInfo();
    present.pNext = nullptr;
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores = &(render_finished_semaphores_[current_frame_]);
    present.swapchainCount = 1;
    present.pSwapchains = &swap_chain_;
    // image_index 是当前渲染的图像索引
    present.pImageIndices = &image_index_;
    present.pResults = nullptr;

    // 切换至下一帧
    current_frame_ = (current_frame_ + 1) % FRAMES_IN_FLIGHT;

    VkResult res = vkQueuePresentKHR(present_queue_, &present);
    return res;
}

uint32_t SwapChain::getFrameCount() const
{
    return FRAMES_IN_FLIGHT;
}

} // yu::vk
