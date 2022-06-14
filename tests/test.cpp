//
// Created by 秋鱼 on 2022/5/6.
//

#define CATCH_CONFIG_MAIN

#include <catch2/catch.hpp>
#include <filesystem>
#include <memory>
#include "win_platform.hpp"
#include "logger.hpp"
#include "filesystem.hpp"
#include "RHI/vulkan/instance_properties.hpp"
#include "RHI/vulkan/instance.hpp"
#include "RHI/vulkan/device.hpp"
#include "RHI/vulkan/swap_chain.hpp"

namespace fs = std::filesystem;
using namespace yu::vk;

TEST_CASE("test", "[TestLog]")
{
    San::LogSystem log;

    San::WinPlatform platform;
    San::Application app{};
    platform.setApplication(&app);
    auto code = platform.initialize();

    {
        VulkanInstance inst{"Test", InstanceProperties{}};
        inst.createSurface(platform.getWindow());

        VulkanDevice device;
        device.create(inst);
        
        SwapChain swapChain{device};
        swapChain.createWindowSizeDependency(inst.getSurface());
        
        
        swapChain.destroyWindowSizeDependency();

        auto [d_name, d_api] = device.getProperties().getDeviceInfo();

        LOG_INFO("Found device: {}. {}", d_name, d_api);
    }

    if (code == San::ExitCode::Success) {
        code = platform.mainLoop();
    }

    platform.terminate(code);
}
