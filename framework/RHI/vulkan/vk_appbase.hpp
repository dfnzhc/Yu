//
// Created by 秋鱼 on 2022/5/11.
//

#pragma once

#include <vulkan/vulkan_core.h>
#include "yuan/platform/Application.hpp"
#include "vk_device.hpp"
#include "vk_swapchain.hpp"

namespace ST::VK {

class AppBase : public Yuan::Application
{
public:
    AppBase() = default;
    virtual ~AppBase() {};

    //--------------------------------------------------------------------------------------------------
    // 基础的设置
    //--------------------------------------------------------------------------------------------------
    /**
     * @brief Prepares the application for execution
     * @param platform The platform the application is being run on
     */
    virtual bool prepare(Yuan::Platform& platform) override;

    /**
     * @brief setup application properties
     */
    virtual void setup() override;

    /**
     * @brief Updates the application
     * @param delta_time The time since the last update
     */
    virtual void update(float delta_time) override;

    /**
     * @brief Handles cleaning up the application
     */
    virtual void finish() override;

    /**
     * @brief Handles resizing of the window
     * @param width New width of the window
     * @param height New height of the window
     */
    virtual bool resize(const uint32_t width, const uint32_t height) override;

    /**
     * @brief Handles input events of the window
     * @param input_event The input event object
     */
    virtual void input_event(const Yuan::InputEvent& input_event) override;

    //--------------------------------------------------------------------------------------------------
    // Vulkan 变量和方法
    //--------------------------------------------------------------------------------------------------
protected:
    struct
    {
        uint32_t apiVersion = VK_API_VERSION_1_3;
    } vulkan_properties_;

    VkInstance instance_{};
    std::vector<std::string> supported_instance_extensions_;

    VkDebugUtilsMessengerEXT Debug_messenger_{};
    // 物理设备 GPU
    VkPhysicalDevice physical_device_{};
    // 物理设备的相关属性和特质
    VkPhysicalDeviceProperties device_properties_{};
    VkPhysicalDeviceFeatures device_features_{};
    VkPhysicalDeviceMemoryProperties device_memory_properties_{};
    VkPhysicalDeviceFeatures enabled_features_{};
    // 启用的设备和实例扩展
    std::vector<const char*> enabled_device_extensions_;
    std::vector<const char*> enabled_instance_extensions_;
    // 在 device 创建的时，可选的 pNext 结构
    void* device_create_pNext = nullptr;

    VkDevice device_{};
    VkQueue queue_{};
    VkFormat depth_format_;
    VkCommandPool cmd_pool_;
    VkPipelineStageFlags submit_pipeline_stages_ = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit_info_{};
    std::vector<VkCommandBuffer> drawCmdBuffers;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> frameBuffers;
    uint32_t currentBuffer = 0;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    std::vector<VkShaderModule> shaderModules;
    VkPipelineCache pipelineCache;

    VulkanSwapChain swap_chain_;

    // 同步信号器
    struct
    {
        // 交换链的图像展示
        VkSemaphore presentComplete;
        // 命令缓冲区的提交和执行
        VkSemaphore renderComplete;
    } semaphores_;
    std::vector<VkFence> waitFences;

public:
    struct
    {
#ifdef NDEBUG
        bool validation = false;
#else
        bool validation = true;
#endif
        bool vsync = false;
    } settings_;

    struct
    {
        VkImage image;
        VkDeviceMemory mem;
        VkImageView view;
    } depthStencil;
    
    uint32_t width_ = 1280;
    uint32_t height_ = 720;

    VulkanDevice* vulkan_device_ = nullptr;
public:
    void initVulkan();
    void createCommandPool();
    void createCommandBuffers();
    void destroyCommandBuffers();
    void createSynchronizationPrimitives();
    void setupDepthStencil();
    void setupRenderPass();
    void createPipelineCache();
    void setupFrameBuffer();

    virtual void setupVulkan();

    virtual void createInstance();
    virtual void getDeviceEnabledFeatures() {}

    void setValidation(bool validation) { settings_.validation = validation; }
};

} // namespace ST
