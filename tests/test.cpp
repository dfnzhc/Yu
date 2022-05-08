//
// Created by 秋鱼 on 2022/5/6.
//

#define CATCH_CONFIG_MAIN

#include <catch2/catch.hpp>
#include <filesystem>
#include "common/logger.hpp"

namespace fs = std::filesystem;

TEST_CASE("test", "[TestLog]")
{
    ST::Log log;

    LOG_TRACE("123");
    LOG_DEBUG("123");
    LOG_INFO("123");
    LOG_WARN("123");
    LOG_ERROR("123");
}