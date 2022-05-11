//
// Created by 秋鱼 on 2022/5/11.
//

#pragma once

#include "yuan/platform/Application.hpp"

namespace ST {

class vk_AppBase : public Yuan::Application
{
public:
    vk_AppBase() = default;
    virtual ~vk_AppBase() = default;

    //--------------------------------------------------------------------------------------------------
    // 基础的设置
    //--------------------------------------------------------------------------------------------------
    /**
     * @brief Prepares the application for execution
     * @param platform The platform the application is being run on
     */
    virtual bool prepare(Platform& platform) override;

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
    virtual void input_event(const InputEvent& input_event) override;

    //--------------------------------------------------------------------------------------------------
    // Vulkan 功能
    //--------------------------------------------------------------------------------------------------
    
};

} // namespace ST
