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
}

SwapChain::~SwapChain()
{
    destroyWindowSizeDependency();
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
    swapchain_info.minImageCount = desiredNumberOfSwapchainImages;
    swapchain_info.imageFormat = color_format_;
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
}

void SwapChain::destroyWindowSizeDependency()
{
    // destroy image view
    for (auto& image_view : image_views_) {
        vkDestroyImageView(device_->getHandle(), image_view, nullptr);
    }

    // 摧毁交换链
    if (swap_chain_ != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device_->getHandle(), swap_chain_, nullptr);
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
        color_format_ = VK_FORMAT_B8G8R8A8_UNORM;
        color_space_ = surfaceFormats[0].colorSpace;
    } else {
        // 遍历可用的 surface 格式列表，并且检查是否存在 VK_FORMAT_B8G8R8A8_UNORM
        bool found_B8G8R8A8_UNORM = false;
        for (auto&& surfaceFormat : surfaceFormats) {
            if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM) {
                color_format_ = surfaceFormat.format;
                color_space_ = surfaceFormat.colorSpace;
                found_B8G8R8A8_UNORM = true;
                break;
            }
        }

        // 当 VK_FORMAT_B8G8R8A8_UNORM 不可用时，选择第一个可用的颜色格式
        if (!found_B8G8R8A8_UNORM) {
            color_format_ = surfaceFormats[0].format;
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
        colorAttachmentView.pNext = NULL;
        colorAttachmentView.format = color_format_;
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

void SwapChain::createFrameBuffers(uint32_t width, uint32_t height)
{
    frame_buffers_.resize(image_count_);
}

} // yu::vk
