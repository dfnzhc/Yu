//
// Created by 秋鱼 on 2022/5/8.
//

#pragma once

#include <cassert>
#include <vulkan/vulkan_core.h>


namespace ST::VK {

bool CheckResult(VkResult result, const char* msg = nullptr);
bool CheckResult(VkResult result, const char* file, int32_t line);

#define VK_CHECK(result) ST::VK::CheckResult(result, __FILE__, __LINE__)

} // namespace ST