//
// Created by 秋鱼 on 2022/6/19.
//

#include "upload_heap.hpp"
#include "initializers.hpp"
#include "error.hpp"
#include "common/math_utils.hpp"

namespace yu::vk {

void UploadHeap::create(const VulkanDevice& device, uint64_t totalSize)
{
    device_ = &device;

    allocating_.reset();
    flushing_.reset();

    // 创建命令池和命令缓冲区
    {
        auto poolInfo = commandPoolCreateInfo();
        poolInfo.queueFamilyIndex = device_->getGraphicsQueueIndex();
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        VK_CHECK(vkCreateCommandPool(device_->getHandle(), &poolInfo, nullptr, &command_pool_));

        auto cmdBufferInfo = commandBufferAllocateInfo(command_pool_, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
        VK_CHECK(vkAllocateCommandBuffers(device_->getHandle(), &cmdBufferInfo, &command_buffer_));
    }

    // 创建用于分配的缓冲区
    {
        VK_CHECK(device_->createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                       totalSize,
                                       &buffer_,
                                       &device_memory_,
                                       false,
                                       (void**) (&data_begin)));
        
        VkMemoryRequirements memReqs;
        vkGetBufferMemoryRequirements(device_->getHandle(), buffer_, &memReqs);

        data_curr = data_begin;
        data_end = data_begin + memReqs.size;
    }

    // 创建用于同步的 fence
    {
        auto fenceInfo = fenceCreateInfo();
        VK_CHECK(vkCreateFence(device_->getHandle(), &fenceInfo, nullptr, &fence_));
    }

    // 开始记录缓冲区命令
    {
        auto beginInfo = commandBufferBeginInfo();
        VK_CHECK(vkBeginCommandBuffer(command_buffer_, &beginInfo));
    }
}

void UploadHeap::destory()
{
    vkDestroyBuffer(device_->getHandle(), buffer_, nullptr);
    vkUnmapMemory(device_->getHandle(), device_memory_);
    vkFreeMemory(device_->getHandle(), device_memory_, nullptr);

    vkFreeCommandBuffers(device_->getHandle(), command_pool_, 1, &command_buffer_);
    vkDestroyCommandPool(device_->getHandle(), command_pool_, nullptr);

    vkDestroyFence(device_->getHandle(), fence_, nullptr);
}

uint8_t* UploadHeap::alloc(uint64_t size, uint64_t align)
{
    flushing_.wait();

    uint8_t* pRet = nullptr;
    {
        std::unique_lock lock{mutex_};

        assert(size < static_cast<uint64_t>(data_begin - data_end));

        data_curr = reinterpret_cast<uint8_t*>(AlignUp(reinterpret_cast<uint64_t>(data_curr), align));
        size = AlignUp(size, align);

        // 如果空间不足，则分配失败
        if (data_curr >= data_end || data_curr + size >= data_end) {
            return nullptr;
        }

        pRet = data_curr;
        data_curr += size;
    }

    return pRet;
}

uint8_t* UploadHeap::beginAlloc(uint64_t size, uint64_t align)
{
    uint8_t* pRet = nullptr;

    // 如果分配失败，则把先前的数据提交，再进行尝试
    while (true) {
        pRet = alloc(size, align);
        if (pRet) {
            break;
        }

        flushAndFinish();
    }

    allocating_.inc();

    return pRet;
}

void UploadHeap::endAlloc()
{
    allocating_.dec();
}

void UploadHeap::addImageCopy(VkImage image, VkBufferImageCopy region)
{
    std::unique_lock lock{mutex_};
    image_copies_.push_back({image, region});
}

void UploadHeap::addImagePreBarrier(VkImageMemoryBarrier imageMemBarrier)
{
    std::unique_lock lock{mutex_};
    pre_barriers_.push_back(imageMemBarrier);
}

void UploadHeap::addImagePostBarrier(VkImageMemoryBarrier imageMemBarrier)
{
    std::unique_lock lock{mutex_};
    post_barriers_.push_back(imageMemBarrier);
}

void UploadHeap::flush()
{
    auto range = mappedMemoryRange();
    range.size = data_curr - data_begin;
    range.memory = device_memory_;

    VK_CHECK(vkFlushMappedMemoryRanges(device_->getHandle(), 1, &range));
}

void UploadHeap::flushAndFinish(bool bDoBarriers)
{
    // 确保别的线程没有在 flush 上传堆
    flushing_.wait();

    // 开始一个 flush 任务
    flushing_.inc();

    // 等待分配任务完成，确保 flush 时不会有分配任务
    allocating_.wait();

    std::unique_lock lock{mutex_};
    flush();

    // 上传图片
    // 实施前置的 barrier
    if (!pre_barriers_.empty()) {
        vkCmdPipelineBarrier(command_buffer_,
                             VK_PIPELINE_STAGE_HOST_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0,
                             0,
                             nullptr,
                             0,
                             nullptr,
                             static_cast<uint32_t>(pre_barriers_.size()),
                             pre_barriers_.data());
        pre_barriers_.clear();
    }

    for (auto& c : image_copies_) {
        vkCmdCopyBufferToImage(command_buffer_,
                               buffer_,
                               c.image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1,
                               &c.region);
    }
    image_copies_.clear();

    // 实施后置的 barrier
    if (!post_barriers_.empty()) {
        vkCmdPipelineBarrier(command_buffer_,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0,
                             0,
                             nullptr,
                             0,
                             nullptr,
                             static_cast<uint32_t>(post_barriers_.size()),
                             post_barriers_.data());
        post_barriers_.clear();
    }

    // 关闭，然后提交命令缓冲区
    VK_CHECK(vkEndCommandBuffer(command_buffer_));

    auto submit_info = submitInfo();
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = nullptr;
    submit_info.pWaitDstStageMask = nullptr;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer_;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = nullptr;

    VK_CHECK(vkQueueSubmit(device_->getGraphicsQueue(), 1, &submit_info, fence_));

    // 等待 GPU 处理完成，然后重置 fence
    VK_CHECK(vkWaitForFences(device_->getHandle(), 1, &fence_, VK_TRUE, UINT64_MAX));
    vkResetFences(device_->getHandle(), 1, &fence_);

    // 重新设置，让命令缓冲区开始记录
    auto beginInfo = commandBufferBeginInfo();
    VK_CHECK(vkBeginCommandBuffer(command_buffer_, &beginInfo));

    data_curr = data_begin;

    // flush 操作完成，计数器减一
    flushing_.dec();
}

} // yu::vk