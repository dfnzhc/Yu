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

    // 创建渲染器
    initRenderer();
    if (!renderer_) {
        LOG_FATAL("Renderer has not been setup");
    }
    renderer_->create(device_.get(), swap_chain_.get());
    
    auto [width, height] = platform_->getWindow()->getExtent();
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
    swap_chain_ = std::make_unique<SwapChain>(*device_);
    swap_chain_->createWindowSizeDependency(instance_->getSurface());
}

InstanceProperties AppBase::setInstanceProps()
{
    return {};
}

} // yu::vk
