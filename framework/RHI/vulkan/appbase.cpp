//
// Created by 秋鱼 on 2022/6/9.
//

#include "appbase.hpp"
#include <platform.hpp>
#include <system_utils.hpp>
#include <logger.hpp>
#include <glfw_window.hpp>
#include <imgui.h>
#include <common/imgui_impl_glfw.h>

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
    auto* glfwWindow = dynamic_cast<San::GLFW_Window*>(platform_->getWindow());
    renderer_->createUI(*instance_, glfwWindow->getGLFWHandle());
    renderer_->createWindowSizeDependency(width, height);
}

void AppBase::update(float delta_time)
{
    Application::update(delta_time);

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    static bool loadingAssets = true;
    if (loadingAssets) {
        static int loadingStage = 0;
        loadingStage = renderer_->loadAssets(loadingStage);
        if (loadingStage == 0) {
            loadingAssets = false;
        }
    } else {
        buildUI();
    }
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

    vkDeviceWaitIdle(device_->getHandle());
    swap_chain_->destroyWindowSizeDependency();
    instance_->createSurface(platform_->getWindow());
    swap_chain_->createWindowSizeDependency(instance_->getSurface());

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

        ImGuiIO& io = ImGui::GetIO();
        if (mouse_event.isEvent(MouseButton::Left, MouseAction::Press) ||
            mouse_event.isEvent(MouseButton::Left, MouseAction::PressedMove)) {
            io.AddMouseButtonEvent(ImGuiMouseButton_Left, true);
        }
        if (mouse_event.isEvent(MouseButton::Left, MouseAction::Release)) {
            io.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
        }
        // (2) ONLY forward mouse data to your underlying app/game.
        if (io.WantCaptureMouse) {
            return;
        }

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

        if (mouse_event.isEvent(MouseButton::Right, MouseAction::PressedMove)) {
            mouse_tracker_->update(static_cast<int>(mouse_event.pos_x),
                                   static_cast<int>(mouse_event.pos_y),
                                   CameraUpdateOp::Offset);
        }

        if (mouse_event.isEvent(MouseButton::Right, MouseAction::Release)) {
            mouse_tracker_->stopTracking();
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

void AppBase::buildUI()
{
    // 渲染性能的 profiler
    auto [W, H] = platform_->getWindow()->getExtent();

    {
        const uint32_t PROFILER_WINDOW_PADDIG_X = 10;
        const uint32_t PROFILER_WINDOW_PADDIG_Y = 10;
        const uint32_t PROFILER_WINDOW_SIZE_X = 400;
        const uint32_t PROFILER_WINDOW_SIZE_Y = 450;
        const uint32_t PROFILER_WINDOW_POS_X = W - PROFILER_WINDOW_PADDIG_X - PROFILER_WINDOW_SIZE_X;
        const uint32_t PROFILER_WINDOW_POS_Y = PROFILER_WINDOW_PADDIG_Y;

        // track highest frame rate and determine the max value of the graph based on the measured highest value
        static float RecentHighestFrameTime = 0.0f;
        static float RecentLowestFrameTime = 99999.0f;
        constexpr int RecordCount = 128;
        static std::vector<float> FrameTimeRecords(RecordCount, 0.0);

        //scrolling data and average FPS computing
        const auto& timeStamps = renderer_->getTimings();
        const bool bTimeStampsAvailable = !timeStamps.empty();
        if (bTimeStampsAvailable) {
            std::rotate(FrameTimeRecords.begin(), FrameTimeRecords.begin() + 1, FrameTimeRecords.end());
            float ms = timeStamps.back().microseconds;
            FrameTimeRecords.back() = ms;

            if (ms < RecentLowestFrameTime) RecentLowestFrameTime = ms;
            if (ms > RecentHighestFrameTime) RecentHighestFrameTime = ms;
        }
        const float frameTime_ms = FrameTimeRecords.back();
        const int fps = bTimeStampsAvailable ? static_cast<int>(1e6f / frameTime_ms) : 0;

        // UI
        ImGui::SetNextWindowPos(ImVec2((float) PROFILER_WINDOW_POS_X, (float) PROFILER_WINDOW_POS_Y), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(PROFILER_WINDOW_SIZE_X, PROFILER_WINDOW_SIZE_Y), ImGuiCond_FirstUseEver);
        ImGui::Begin("PROFILER");

        ImGui::Text("Resolution : %ix%i", W, H);
        ImGui::Text("API        : %s", system_info_.APIVersion.c_str());
        ImGui::Text("GPU        : %s", system_info_.GPUName.c_str());
        ImGui::Text("CPU        : %s", system_info_.CPUName.c_str());
        ImGui::Text("FPS        : %d (%.2f ms)", fps, frameTime_ms);

        if (ImGui::CollapsingHeader("GPU Timings", ImGuiTreeNodeFlags_DefaultOpen)) {
            // WARNING: If validation layer is switched on, the performance numbers may be inaccurate!
            
            ImGui::PlotLines("",
                             FrameTimeRecords.data(),
                             RecordCount,
                             0,
                             "GPU frame time (ms)",
                             RecentLowestFrameTime,
                             RecentHighestFrameTime,
                             ImVec2(0, 80));

            for (const auto& timeStamp : timeStamps) {
                float value = timeStamp.microseconds / 1000.0f;
                ImGui::Text("%-18s: %7.2f ms", timeStamp.label.c_str(), value);
            }
        }
        ImGui::End(); // PROFILER
    }
}

InstanceProperties AppBase::setInstanceProps()
{
    return {};
}

} // yu::vk
