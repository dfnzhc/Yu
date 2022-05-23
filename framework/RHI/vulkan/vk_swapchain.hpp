//
// Created by 秋鱼 on 2022/5/20.
//

#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

namespace ST::VK {

class VulkanSwapChain
{
    VkInstance instance;
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkSurfaceKHR surface;
    // Function pointers
    PFN_vkGetPhysicalDeviceSurfaceSupportKHR fpGetPhysicalDeviceSurfaceSupportKHR;
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR fpGetPhysicalDeviceSurfaceCapabilitiesKHR;
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR fpGetPhysicalDeviceSurfaceFormatsKHR;
    PFN_vkGetPhysicalDeviceSurfacePresentModesKHR fpGetPhysicalDeviceSurfacePresentModesKHR;
    PFN_vkCreateSwapchainKHR fpCreateSwapchainKHR;
    PFN_vkDestroySwapchainKHR fpDestroySwapchainKHR;
    PFN_vkGetSwapchainImagesKHR fpGetSwapchainImagesKHR;
    PFN_vkAcquireNextImageKHR fpAcquireNextImageKHR;
    PFN_vkQueuePresentKHR fpQueuePresentKHR;

public:
    VkFormat colorFormat;
    VkColorSpaceKHR colorSpace;
    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    uint32_t imageCount;
    std::vector<VkImage> images;
    uint32_t queueNodeIndex = UINT32_MAX;

    struct SwapChainBuffer
    {
        VkImage image;
        VkImageView view;
    };
    std::vector<SwapChainBuffer> buffers;

    void initSurface(GLFWwindow* window);

    void connect(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device);
    void create(uint32_t* width, uint32_t* height, bool vsync = false);
    VkResult acquireNextImage(VkSemaphore presentCompleteSemaphore, uint32_t* imageIndex);
    VkResult queuePresent(VkQueue queue, uint32_t imageIndex, VkSemaphore waitSemaphore = VK_NULL_HANDLE);
    void cleanup();
};

} // namespace ST::VK
