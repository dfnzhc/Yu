//
// Created by 秋鱼 on 2022/5/20.
//

#include "vk_swapchain.hpp"
#include "vk_error.hpp"

namespace ST::VK {

void VulkanSwapChain::initSurface(GLFWwindow* window)
{
    LOG_INFO("\tCreate surface...");
    VK_CHECK(glfwCreateWindowSurface(instance, window, nullptr, &surface));

    // 获取可用的队列族属性
    uint32_t queueCount;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, nullptr);
    assert(queueCount >= 1);

    std::vector<VkQueueFamilyProperties> queueProps(queueCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, queueProps.data());

    // 遍历每个队列，了解它是否支持显示:
    // 找到一个支持显示的队列用于在窗口系统上显示交换链图像
    std::vector<VkBool32> supportsPresent(queueCount);
    for (uint32_t i = 0; i < queueCount; i++) {
        fpGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &supportsPresent[i]);
    }

    // 在队列数组中搜索图形和显示的队列家族，尝试找到一个同时支持两者的队列
    uint32_t graphicsQueueNodeIndex = UINT32_MAX;
    uint32_t presentQueueNodeIndex = UINT32_MAX;
    for (uint32_t i = 0; i < queueCount; i++) {
        if ((queueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
            if (graphicsQueueNodeIndex == UINT32_MAX) {
                graphicsQueueNodeIndex = i;
            }

            if (supportsPresent[i] == VK_TRUE) {
                graphicsQueueNodeIndex = i;
                presentQueueNodeIndex = i;
                break;
            }
        }
    }
    if (presentQueueNodeIndex == UINT32_MAX) {
        // 如果没有同时支持显示和图形的队列, 尝试找到一个单独的显示队列
        for (uint32_t i = 0; i < queueCount; ++i) {
            if (supportsPresent[i] == VK_TRUE) {
                presentQueueNodeIndex = i;
                break;
            }
        }
    }

    // Exit if either a graphics or a presenting queue hasn't been found
    if (graphicsQueueNodeIndex == UINT32_MAX || presentQueueNodeIndex == UINT32_MAX) {
        throw std::runtime_error("Could not find a graphics and/or presenting queue!");
    }

    // todo : Add support for separate graphics and presenting queue
    if (graphicsQueueNodeIndex != presentQueueNodeIndex) {
        throw std::runtime_error("Separate graphics and presenting queues are not supported yet!");
    }

    queueNodeIndex = graphicsQueueNodeIndex;

    // 获取支持的 surface 格式
    uint32_t formatCount;
    VK_CHECK(fpGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr));
    assert(formatCount > 0);

    std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
    VK_CHECK(fpGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, surfaceFormats.data()));

    // 如果 surface 格式列表中只有一个带有 VK_FORMAT_UNDEFINED 的条目,
    // 那就没有首选的格式，则假定使用 VK_FORMAT_B8G8R8A8_UNORM
    if ((formatCount == 1) && (surfaceFormats[0].format == VK_FORMAT_UNDEFINED)) {
        colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
        colorSpace = surfaceFormats[0].colorSpace;
    } else {
        // 遍历可用的 surface 格式列表，并且检查是否存在 VK_FORMAT_B8G8R8A8_UNORM
        bool found_B8G8R8A8_UNORM = false;
        for (auto&& surfaceFormat : surfaceFormats) {
            if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM) {
                colorFormat = surfaceFormat.format;
                colorSpace = surfaceFormat.colorSpace;
                found_B8G8R8A8_UNORM = true;
                break;
            }
        }

        // 当 VK_FORMAT_B8G8R8A8_UNORM 不可用时，选择第一个可用的颜色格式
        if (!found_B8G8R8A8_UNORM) {
            colorFormat = surfaceFormats[0].format;
            colorSpace = surfaceFormats[0].colorSpace;
        }
    }
}

/**
* 设置用于交换链的 vulkan实例、物理和逻辑设备，并获得所有需要的函数指针
* 
* @param instance 要使用的Vulkan实例
* @param physicalDevice 用于查询 swapchain 相关属性和格式的物理设备
* @param device 用于创建交换链的逻辑设备
*
*/
void VulkanSwapChain::connect(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device)
{
    
    this->instance = instance;
    this->physicalDevice = physicalDevice;
    this->device = device;
    fpGetPhysicalDeviceSurfaceSupportKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceSupportKHR>(
        vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceSupportKHR"));

    fpGetPhysicalDeviceSurfaceCapabilitiesKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR>(
        vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR"));

    fpGetPhysicalDeviceSurfaceFormatsKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceFormatsKHR>(
        vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceFormatsKHR"));

    fpGetPhysicalDeviceSurfacePresentModesKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfacePresentModesKHR>(
        vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfacePresentModesKHR"));

    fpCreateSwapchainKHR = reinterpret_cast<PFN_vkCreateSwapchainKHR>(vkGetDeviceProcAddr(device, "vkCreateSwapchainKHR"));
    fpDestroySwapchainKHR = reinterpret_cast<PFN_vkDestroySwapchainKHR>(vkGetDeviceProcAddr(device, "vkDestroySwapchainKHR"));
    fpGetSwapchainImagesKHR = reinterpret_cast<PFN_vkGetSwapchainImagesKHR>(vkGetDeviceProcAddr(device, "vkGetSwapchainImagesKHR"));
    fpAcquireNextImageKHR = reinterpret_cast<PFN_vkAcquireNextImageKHR>(vkGetDeviceProcAddr(device, "vkAcquireNextImageKHR"));
    fpQueuePresentKHR = reinterpret_cast<PFN_vkQueuePresentKHR>(vkGetDeviceProcAddr(device, "vkQueuePresentKHR"));
}

/** 
* 创建交换链，给定交换链图像的宽度和高度
* 
* @param width 指向交换链的宽度（可根据交换链的要求进行调整）
* @param height 指向交换链的高度（可根据交换链的要求进行调整）
* @param vsync 可以用来强制进行 vsync-ed 渲染（使用 VK_PRESENT_MODE_FIFO_KHR 作为呈现模式）
*/
void VulkanSwapChain::create(uint32_t* width, uint32_t* height, bool vsync)
{
    // 存储当前的交换链句柄，以便之后用它来进行重建交换链
    VkSwapchainKHR oldSwapchain = swapChain;

    // 获取物理设备的 surface 属性和格式
    VkSurfaceCapabilitiesKHR surfCaps;
    VK_CHECK(fpGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfCaps));

    // 获取可用的显示模式
    uint32_t presentModeCount;
    VK_CHECK(fpGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr));
    assert(presentModeCount > 0);

    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    VK_CHECK(fpGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data()));

    VkExtent2D swapchainExtent = {};
    // 如果宽度（和高度）等于特殊值0xFFFFFFFF，表面的大小将由交换链来设置
    if (surfCaps.currentExtent.width == (uint32_t) -1) {
        // 如果不去定义 surface 尺寸，则尺寸被设置为所请求的图像的大小
        swapchainExtent.width = *width;
        swapchainExtent.height = *height;
    } else {
        // 如果定义了 surface 尺寸，交换链尺寸必须匹配
        swapchainExtent = surfCaps.currentExtent;
        *width = surfCaps.currentExtent.width;
        *height = surfCaps.currentExtent.height;
    }


    // 为交换链选择一个显示模式

    // 默认使用 VK_PRESENT_MODE_FIFO_KHR 模式
    // 这种模式会等待垂直同步（"v-sync"）
    VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;

    // 如果不要求 v-sync，则尝试找到一个邮箱模式
    // 这是可用的延迟最低的非撕裂式存在模式
    if (!vsync) {
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

    // 确定图像的数量
    uint32_t desiredNumberOfSwapchainImages = surfCaps.minImageCount + 1;
    if ((surfCaps.maxImageCount > 0) && (desiredNumberOfSwapchainImages > surfCaps.maxImageCount)) {
        desiredNumberOfSwapchainImages = surfCaps.maxImageCount;
    }

    // 找出 surface 的变换
    VkSurfaceTransformFlagsKHR preTransform;
    if (surfCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        // 更倾向于采用非旋转的变换
        preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    } else {
        preTransform = surfCaps.currentTransform;
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
        if (surfCaps.supportedCompositeAlpha & compositeAlphaFlag) {
            compositeAlpha = compositeAlphaFlag;
            break;
        }
    }

    VkSwapchainCreateInfoKHR swapchainCI = {};
    swapchainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCI.surface = surface;
    swapchainCI.minImageCount = desiredNumberOfSwapchainImages;
    swapchainCI.imageFormat = colorFormat;
    swapchainCI.imageColorSpace = colorSpace;
    swapchainCI.imageExtent = {swapchainExtent.width, swapchainExtent.height};
    swapchainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainCI.preTransform = (VkSurfaceTransformFlagBitsKHR) preTransform;
    swapchainCI.imageArrayLayers = 1;
    swapchainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainCI.queueFamilyIndexCount = 0;
    swapchainCI.presentMode = swapchainPresentMode;
    // 将oldSwapChain设置为前一个交换链的保存柄，有助于资源的再利用，并确保我们仍然可以展示已经获得的图像
    swapchainCI.oldSwapchain = oldSwapchain;
    // 将clipped设置为VK_TRUE，允许实现放弃 surface 区域以外的渲染
    swapchainCI.clipped = VK_TRUE;
    swapchainCI.compositeAlpha = compositeAlpha;

    // 如果支持，在交换链图像上启用传输源
    if (surfCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
        swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }

    // 如果支持的话，在交换链图像上启用传输目标
    if (surfCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
        swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }

    VK_CHECK(fpCreateSwapchainKHR(device, &swapchainCI, nullptr, &swapChain));

    // 如果一个现有的交换链被重新创建，则销毁旧的交换链
    // 这也是对所有可呈现的图像进行清理
    if (oldSwapchain != VK_NULL_HANDLE) {
        for (uint32_t i = 0; i < imageCount; i++) {
            vkDestroyImageView(device, buffers[i].view, nullptr);
        }
        fpDestroySwapchainKHR(device, oldSwapchain, nullptr);
    }
    VK_CHECK(fpGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr));

    // 获取互换链图像
    images.resize(imageCount);
    VK_CHECK(fpGetSwapchainImagesKHR(device, swapChain, &imageCount, images.data()));

    // 获取包含图像和图像视图的交换链缓冲区
    buffers.resize(imageCount);
    for (uint32_t i = 0; i < imageCount; i++) {
        VkImageViewCreateInfo colorAttachmentView = {};
        colorAttachmentView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        colorAttachmentView.pNext = nullptr;
        colorAttachmentView.format = colorFormat;
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

        buffers[i].image = images[i];

        colorAttachmentView.image = buffers[i].image;

        VK_CHECK(vkCreateImageView(device, &colorAttachmentView, nullptr, &buffers[i].view));
    }
}

/** 
* 获取交换链中的下一个图像
*
* @param presentCompleteSemaphore (Optional) 当图像可以使用时，会发出同步信号
* @param imageIndex 指向图像索引的指针，如果可以获得下一个图像，该索引将被增加
*
* @note 该函数将一直等待，直到下一个图像被获取，将timeout设置为UINT64_MAX
*
* @return VkResult of the image acquisition
*/
VkResult VulkanSwapChain::acquireNextImage(VkSemaphore presentCompleteSemaphore, uint32_t* imageIndex)
{
    // 通过设置超时为 UINT64_MAX，我们将一直等待，直到获得下一个图像或抛出一个实际错误
    // 这样就不需要处理 VK_NOT_READY 了
    return fpAcquireNextImageKHR(device, swapChain, UINT64_MAX, presentCompleteSemaphore, (VkFence) nullptr, imageIndex);
}

/**
* 排队需要显示的图像
*
* @param queue 用于显示的图像的队列
* @param imageIndex 要排队显示的交换链中的图像索引
* @param waitSemaphore (Optional) 在图像呈现前等待的信号量（仅在 != VK_NULL_HANDLE时使用）
*
* @return VkResult of the queue presentation
*/
VkResult VulkanSwapChain::queuePresent(VkQueue queue, uint32_t imageIndex, VkSemaphore waitSemaphore)
{
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapChain;
    presentInfo.pImageIndices = &imageIndex;
    // 检查是否已经指定了一个等待信号，以便在呈现图像之前进行等待
    if (waitSemaphore != VK_NULL_HANDLE) {
        presentInfo.pWaitSemaphores = &waitSemaphore;
        presentInfo.waitSemaphoreCount = 1;
    }
    return fpQueuePresentKHR(queue, &presentInfo);
}

/**
* 销毁并释放用于交换链的 Vulkan 资源
*/
void VulkanSwapChain::cleanup()
{
    if (swapChain != VK_NULL_HANDLE) {
        for (uint32_t i = 0; i < imageCount; i++) {
            vkDestroyImageView(device, buffers[i].view, nullptr);
        }
    }
    if (surface != VK_NULL_HANDLE) {
        fpDestroySwapchainKHR(device, swapChain, nullptr);
        vkDestroySurfaceKHR(instance, surface, nullptr);
    }
    surface = VK_NULL_HANDLE;
    swapChain = VK_NULL_HANDLE;
}
} // namespace ST::VK