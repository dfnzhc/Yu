//
// Created by 秋鱼 on 2022/7/27.
//

#pragma once

#include "device.hpp"
namespace yu::vk {

struct TimeStamp
{
    std::string label;
    float microseconds;
};

class GPUTimeStamp
{
public:
    void create(const VulkanDevice& device, uint32_t numberOfBackBuffers);
    void destroy();
    
    void getTimeStamp(VkCommandBuffer cmdBuffer, std::string_view label);
    void setCpuTimeStamp(TimeStamp ts);
    
    void beginFrame(VkCommandBuffer cmdBuffer, std::vector<TimeStamp>& timeStamp);
    void endFrame();

private:
    const VulkanDevice* device_ = nullptr;
    const uint32_t MaxQueryCountPerFrame = 128;

    VkQueryPool query_pool_{};

    uint32_t frame_ = 0;
    uint32_t backBuffer_count_ = 0;

    std::vector<std::string> labels_[3];
    std::vector<TimeStamp> cpu_time_stamps_[3];
};

} // yu::vk