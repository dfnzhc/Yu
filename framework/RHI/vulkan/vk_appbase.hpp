//
// Created by 秋鱼 on 2022/5/11.
//

#pragma once

#include <vulkan/vulkan_core.h>
#include "yuan/platform/Application.hpp"
#include "vk_device.hpp"
#include "vk_swapchain.hpp"
#include "common/camera.hpp"

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

protected:
    //--------------------------------------------------------------------------------------------------
    // Vulkan 相关变量
    //--------------------------------------------------------------------------------------------------
    struct
    {
        uint32_t apiVersion = VK_API_VERSION_1_3;
    } vulkan_properties_;

    /// Vulkan 实例相关 ------------------------------------------------------------
    VkInstance instance_{};
    std::vector<std::string> supported_instance_extensions_;
    VkDebugUtilsMessengerEXT Debug_messenger_{};
    // 实例扩展
    std::vector<const char*> enabled_instance_extensions_;

    /// Vulkan 设备相关 ------------------------------------------------------------
    // 物理设备(GPU)
    VkPhysicalDevice physical_device_{};
    // 物理设备的相关属性和特性
    VkPhysicalDeviceProperties device_properties_{};
    VkPhysicalDeviceFeatures device_features_{};
    VkPhysicalDeviceMemoryProperties device_memory_properties_{};
    VkPhysicalDeviceFeatures enabled_features_{};
    // 设备扩展
    std::vector<const char*> enabled_device_extensions_;
    // 在 device 创建时，可选的 pNext 结构
    void* device_create_pNext = nullptr;
    // 逻辑设备 与 队列
    VkDevice device_{};
    VkQueue queue_{};

    /// surface 和交换链相关---------------------------------------------------------
    VulkanSwapChain swap_chain_;

    /// 深度、模板缓冲区视图-----------------------------------------------------------
    VkFormat depth_format_;
    struct
    {
        VkImage image;
        VkDeviceMemory mem;
        VkImageView view;
    } depthStencil;

    /// 流水线相关------------------------------------------------------------------
    // 着色器模组
    VkPipelineCache pipeline_cache_;
    std::vector<VkShaderModule> shader_modules_;
    VkPipelineLayout pipeline_layout_;
    VkPipeline pipeline_;
    VkDescriptorSetLayout descriptor_set_layout_;

    VkCommandPool cmd_pool_;
    std::vector<VkCommandBuffer> drawCmdBuffers;
    VkRenderPass render_pass_ = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> frame_buffers_;
    VkSubmitInfo submit_info_{};
    VkPipelineStageFlags submit_pipeline_stages_ = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    uint32_t current_buffer_ = 0;
    VkDescriptorSet descriptor_set_;
    VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;

    VulkanBuffer uniform_buffer_;

    // 同步信号器
    struct
    {
        // 交换链的图像展示
        VkSemaphore presentComplete;
        // 命令缓冲区的提交和执行
        VkSemaphore renderComplete;
    } semaphores_;
    std::vector<VkFence> waitFences;

    //--------------------------------------------------------------------------------------------------
    // Vulkan 的初始化和基础设施的创建
    //--------------------------------------------------------------------------------------------------
    void initVulkan();
    void createInstance();
    void createSwapChain();

    virtual void prepareVulkan();
    virtual void createCommandPool();
    virtual void createCommandBuffers();
    virtual void destroyCommandBuffers();
    virtual void createSynchronizationPrimitives();
    virtual void createDepthStencilView();
    virtual void createPipelineCache();
    virtual void setupRenderPass();
    virtual void setupFrameBuffer();

    virtual void setupUniformBuffers();
    virtual void updateUniformBuffers();
    virtual void setupDescriptorSetLayout();

    virtual void setupPipeline();

    virtual void setupDescriptorPool();
    virtual void setupDescriptorSet();

    virtual void buildCommandBuffer();
    virtual void draw();
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

    Camera camera_;
    bool start_tracking_ = false;
    float xPos_, yPos_;
    
    
    uint32_t width_ = 1280;
    uint32_t height_ = 720;

    VulkanDevice* vulkan_device_ = nullptr;
public:
    VkPipelineShaderStageCreateInfo loadShader(std::string_view fileName);

    virtual void setInstanceExtensions() {}
    virtual void setDeviceExtensions() {}
    virtual void getDeviceEnabledFeatures() {}
    void setValidation(bool validation) { settings_.validation = validation; }
};

} // namespace ST
