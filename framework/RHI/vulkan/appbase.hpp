//
// Created by 秋鱼 on 2022/6/9.
//

#pragma once

#include <Application.hpp>
#include <common/mouse_tracker.hpp>

#include "renderer.hpp"

namespace yu::vk {

class AppBase : public San::Application
{
public:
    ~AppBase() override = default;
    bool prepare(San::Platform& platform) override;
    
    void setup() override;
    void update(float delta_time) override;
    void finish() override;
    
    bool resize(uint32_t width, uint32_t height) override;
    void input_event(const San::InputEvent& input_event) override;

protected:
    void initVulkan();
    
    virtual void initRenderer() {}
    virtual void render() {}

    virtual InstanceProperties setInstanceProps();
    
protected:
    std::unique_ptr<Renderer> renderer_ = nullptr;
    
    std::unique_ptr<VulkanInstance> instance_ = nullptr;
    
    std::unique_ptr<VulkanDevice> device_ = nullptr;
    std::unique_ptr<SwapChain> swap_chain_ = nullptr;
    
    std::unique_ptr<yu::MouseTracker> mouse_tracker_ = nullptr;

    struct
    {
        std::string CPUName = "UNAVAILABLE";
        std::string GPUName = "UNAVAILABLE";
        std::string APIVersion = "UNAVAILABLE";
    } system_info_;
};

} // yu::vk