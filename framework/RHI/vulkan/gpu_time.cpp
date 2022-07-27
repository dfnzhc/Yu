//
// Created by 秋鱼 on 2022/7/27.
//

#include "gpu_time.hpp"
#include "error.hpp"
#include "initializers.hpp"

namespace yu::vk {

void GPUTimeStamp::create(const VulkanDevice& device, uint32_t numberOfBackBuffers)
{
    device_ = &device;
    backBuffer_count_ = numberOfBackBuffers;

    auto createInfo = VkQueryPoolCreateInfo{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
    createInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
    createInfo.queryCount = MaxQueryCountPerFrame * numberOfBackBuffers;

    VK_CHECK(vkCreateQueryPool(device.getHandle(), &createInfo, nullptr, &query_pool_));
}

void GPUTimeStamp::destroy()
{
    vkDestroyQueryPool(device_->getHandle(), query_pool_, nullptr);
}

void GPUTimeStamp::getTimeStamp(VkCommandBuffer cmdBuffer, std::string_view label)
{
    auto measurementCount = static_cast<uint32_t>(labels_[frame_].size());
    auto offset = frame_ * MaxQueryCountPerFrame + measurementCount;
    
    vkCmdWriteTimestamp(cmdBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, query_pool_, offset);
    
    labels_[frame_].emplace_back(label.data());
}

void GPUTimeStamp::setCpuTimeStamp(TimeStamp ts)
{
    cpu_time_stamps_[frame_].push_back(std::move(ts));
}

void GPUTimeStamp::beginFrame(VkCommandBuffer cmdBuffer, std::vector<TimeStamp>& timeStamp)
{
    auto& cpuTimeStamps = cpu_time_stamps_[frame_];
    auto& gpuLabels = labels_[frame_];

    timeStamp.clear();

    // 拷贝 CPU 的测量时间
    if (!cpuTimeStamps.empty()) {
        timeStamp.resize(cpuTimeStamps.size());
        std::copy(std::execution::par, cpuTimeStamps.begin(), cpuTimeStamps.end(), timeStamp.begin());
    }

    // 拷贝 GPU 的测量时间
    uint32_t offset = frame_ * MaxQueryCountPerFrame;
    auto measurementCount = static_cast<uint32_t>(gpuLabels.size());
    if (measurementCount > 0) {
        // timestampPeriod 表示每次 tick 的时间间隔，以纳秒为单位，所以转换成毫秒
        const double msPerTick = device_->getProperties().device_properties.limits.timestampPeriod * 1e-3f;
        {
            std::vector<uint64_t> timingsInTick(measurementCount, 0);
            auto result = vkGetQueryPoolResults(device_->getHandle(),
                                                query_pool_,
                                                offset,
                                                measurementCount,
                                                measurementCount * sizeof(uint64_t),
                                                timingsInTick.data(),
                                                sizeof(uint64_t),
                                                VK_QUERY_RESULT_64_BIT);

            if (result == VK_SUCCESS) {
                // 每个测量部分的时间
                for (uint32_t i = 1; i < measurementCount; i++) {
                    auto ts = TimeStamp{gpuLabels[i], static_cast<float>(msPerTick * static_cast<double>(timingsInTick[i] - timingsInTick[i - 1]))};
                    timeStamp.push_back(std::move(ts));
                }

                // 总的测量时间
                auto ts = TimeStamp{"Total GPU Time",
                                    static_cast<float>(msPerTick * static_cast<double>(timingsInTick[measurementCount - 1] - timingsInTick[0]))};
                timeStamp.push_back(std::move(ts));
            } else {
                timeStamp.emplace_back(TimeStamp{"GPU counters are invalid", 0.0f});
            }
        }
    }
    
    // 查询之后重置 query pool
    vkCmdResetQueryPool(cmdBuffer, query_pool_, offset, MaxQueryCountPerFrame);
    
    cpuTimeStamps.clear();
    gpuLabels.clear();
    
    // 添加一个记录作为起始测量时间点
    getTimeStamp(cmdBuffer, "Begin Frame");
}

void GPUTimeStamp::endFrame()
{
    frame_ = (frame_ + 1) % backBuffer_count_;
}

} // yu::vk