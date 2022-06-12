//
// Created by 秋鱼 on 2022/6/12.
//

#pragma once

#include <ring.hpp>

namespace yu {

class BufferRing
{
public:
    void create(uint32_t numberOfBackBuffers, uint32_t totalSize)
    {
        backBuffer_index_ = 0;
        backBuffers_count_ = numberOfBackBuffers;

        mem_allocated_in_frame_ = 0;
        allocated_mem_per_backBuffer_ = std::vector<uint32_t>(numberOfBackBuffers, 0);

        mem_.create(totalSize);
    }

    void destroy()
    {
        mem_.free(mem_.getSize());
    }

    bool alloc(uint32_t size, uint32_t* offset)
    {
        // 先申请 pad 的内存，防止内存跨越环形缓冲区首尾
        uint32_t pad = mem_.getPaddingToAvoidCrossOver(size);
        if (pad > 0) {
            mem_allocated_in_frame_ += pad;

            if (!mem_.alloc(pad)) {
                LOG_ERROR("No memory to allocate");
                return false;
            }
        }

        // 申请相应大小的内存，并返回它在环形缓冲区中的偏移
        if (mem_.alloc(size, offset)) {
            mem_allocated_in_frame_ += size;
            return true;
        }
        return false;
    }

    void beginFrame()
    {
        allocated_mem_per_backBuffer_[backBuffer_index_] = mem_allocated_in_frame_;
        mem_allocated_in_frame_ = 0;
        
        backBuffer_index_ = (backBuffer_index_ + 1) % backBuffers_count_;
        
        mem_.free(allocated_mem_per_backBuffer_[backBuffer_index_]);
    }

private:
    San::Ring mem_;

    uint32_t backBuffer_index_{};
    uint32_t backBuffers_count_{};

    uint32_t mem_allocated_in_frame_{};
    std::vector<uint32_t> allocated_mem_per_backBuffer_{};
};

} // yu