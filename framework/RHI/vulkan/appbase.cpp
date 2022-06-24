//
// Created by 秋鱼 on 2022/6/9.
//

#include "appbase.hpp"
#include <platform.hpp>
#include <system_utils.hpp>
#include <logger.hpp>

namespace yu::vk {

bool AppBase::prepare(San::Platform& platform)
{
    return Application::prepare(platform);
}

void AppBase::setup()
{
    Application::setup();

    initVulkan();

    LOG_INFO("Successfully set up vulkan");
    LOG_INFO("CPU: {}", system_info_.CPUName);
    LOG_INFO("GPU: {}", system_info_.GPUName);
    LOG_INFO("API: {}", system_info_.APIVersion);

    auto [width, height] = platform_->getWindow()->getExtent();

    // 创建鼠标的追踪类
    mouse_tracker_ = std::make_unique<yu::MouseTracker>();
    mouse_tracker_->camera_->setFov(glm::radians(60.0f), width, height, 0.1f, 1000.0f);
    mouse_tracker_->camera_->lookAt({0, 0, 5}, {0, 0, 0});

    // 创建渲染器
    initRenderer();
    if (!renderer_) {
        LOG_FATAL("Renderer has not been setup");
    }
    renderer_->create(*device_, swap_chain_.get(), *mouse_tracker_);
    renderer_->createWindowSizeDependency(width, height);
}

void AppBase::update(float delta_time)
{
    Application::update(delta_time);
}

void AppBase::finish()
{
    Application::finish();

    vkDeviceWaitIdle(device_->getHandle());

    renderer_->destroyWindowSizeDependency();
    renderer_->destroy();

    swap_chain_->destroyWindowSizeDependency();
}

bool AppBase::resize(uint32_t width, uint32_t height)
{
    if (!Application::resize(width, height)) {
        return false;
    }

    renderer_->resize(width, height);

    return true;
}

void AppBase::input_event(const San::InputEvent& input_event)
{
    Application::input_event(input_event);

    using San::InputEvent;
    using San::EventType;
    using San::KeyInputEvent;
    using San::MouseInputEvent;
    using San::MouseButton;
    using San::MouseAction;

    if (input_event.type == EventType::Keyboard) {
        [[maybe_unused]]
        const auto& key_event = static_cast<const KeyInputEvent&>(input_event);

//        LOG_INFO("[Keyboard] key: {}, action: {}", static_cast<int>(key_event.code), GetActionString(key_event.action));
    } else if (input_event.type == EventType::Mouse) {
        [[maybe_unused]]
        const auto& mouse_event = static_cast<const MouseInputEvent&>(input_event);

        if (mouse_event.isEvent(MouseButton::Left, MouseAction::PressedMove)) {
            mouse_tracker_->update(static_cast<int>(mouse_event.pos_x),
                                   static_cast<int>(mouse_event.pos_y));
        }

        if (mouse_event.isEvent(MouseButton::Left, MouseAction::Release)) {
            mouse_tracker_->stopTracking();
        }
        
        if (mouse_event.isEvent(MouseButton::Middle, MouseAction::Scroll)) {
            mouse_tracker_->zoom(mouse_event.scroll_dir);
        }
//        LOG_INFO("[Mouse] key: {}, action: {} ({}.{} {})",
//                 GetButtonString(mouse_event.button),
//                 GetActionString(mouse_event.action), mouse_event.pos_x, mouse_event.pos_y, mouse_event.scroll_dir);
    }
}

void AppBase::initVulkan()
{
    // 1. 创建 vulkan 实例
    instance_ = std::make_unique<VulkanInstance>(name_, setInstanceProps());
    instance_->createSurface(platform_->getWindow());

    // 2. 创建 vulkan 设备
    device_ = std::make_unique<VulkanDevice>();
    device_->create(*instance_);
    // 获取设备信息
    system_info_.CPUName = San::GetCPUNameString();
    std::tie(system_info_.GPUName, system_info_.APIVersion) = device_->getProperties().getDeviceInfo();

    // 3. 创建交换链
    swap_chain_ = std::make_unique<SwapChain>(*device_, bSwapChain_CreateDepth);
    swap_chain_->createWindowSizeDependency(instance_->getSurface());
}

InstanceProperties AppBase::setInstanceProps()
{
    return {};
}

} // yu::vk
