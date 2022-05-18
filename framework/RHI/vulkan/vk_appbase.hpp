//
// Created by 秋鱼 on 2022/5/11.
//

#pragma once

#include <vulkan/vulkan_core.h>
#include "yuan/platform/Application.hpp"

namespace ST {

class vk_AppBase : public Yuan::Application
{
public:
    vk_AppBase() = default;
    virtual ~vk_AppBase() {};

    //--------------------------------------------------------------------------------------------------
    // 基础的设置
    //--------------------------------------------------------------------------------------------------
    /**
     * @brief Prepares the application for execution
     * @param platform The platform the application is being run on
     */
    virtual bool prepare(Yuan::Platform& platform) override;

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

    struct
    {
#ifdef NDEBUG
        bool validation = false;
#else
        bool validation = true;
#endif
    } vulkan_settings_;

    VkInstance instance_{};
    std::vector<std::string> supported_instance_extensions_;
    // 物理设备 GPU
    VkPhysicalDevice physical_device_{};
    // 物理设备的相关属性和特质
    VkPhysicalDeviceProperties device_properties_{};
    VkPhysicalDeviceFeatures device_features_{};
    VkPhysicalDeviceMemoryProperties device_memory_properties_{};
    VkPhysicalDeviceFeatures enabled_features{};
    // 启用的设备和实例扩展
    std::vector<const char*> enabled_device_extensions_;
    std::vector<const char*> enabled_instance_extensions_;
    // 在 device 创建的时，可选的 pNext 结构
    void* device_create_pNext = nullptr;

public:
    void initVulkan();

    virtual void createInstance();
    
    void setValidation(bool validation) { vulkan_settings_.validation = validation; }
};

} // namespace ST
