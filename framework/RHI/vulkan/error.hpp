//
// Created by 秋鱼 on 2022/6/1.
//

#pragma once

#include <cassert>
#include <vulkan/vulkan_core.h>

namespace yu::vk {

bool CheckResult(VkResult result, const char* msg = nullptr);
bool CheckResult(VkResult result, const char* file, int32_t line);

#define VK_CHECK(result) yu::vk::CheckResult(result, __FILE__, __LINE__)

} // yu::vk