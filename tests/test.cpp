//
// Created by 秋鱼 on 2022/5/6.
//

#define CATCH_CONFIG_MAIN

#include <catch2/catch.hpp>
#include <filesystem>
#include "win_platform.hpp"
#include "logger.hpp"
#include "RHI/vulkan/vk_appbase.hpp"

namespace fs = std::filesystem;

TEST_CASE("test", "[TestLog]")
{
    Yuan::Log::Init();
    
    Yuan::WinPlatform platform;
    
    ST::vk_AppBase vk_app{};

    platform.setApplication(&vk_app);
    
    auto code = platform.initialize();
    if (code == Yuan::ExitCode::Success) {
        code = platform.mainLoop();
    }
    
    platform.terminate(code);
}