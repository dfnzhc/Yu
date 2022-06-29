//
// Created by 秋鱼 on 2022/6/25.
//

#pragma once

#include <GLFW/glfw3.h>
#include "device.hpp"
#include "dynamic_buffer.hpp"
#include "upload_heap.hpp"
#include "pipeline_builder.hpp"
#include "imgui_impl_vulkan.h"
namespace yu::vk {

class ImGUI
{
public:
    ImGUI();
    ~ImGUI();
    void create(const VulkanInstance& instance,
                const VulkanDevice& device,
                VkRenderPass renderPass,
                UploadHeap& uploadHeap,
                GLFWwindow* window);
    void destroy();
    
    void draw(VkCommandBuffer cmdBuffer);

private:
    const VulkanInstance* instance_ = nullptr;
    VulkanDevice* device_ = nullptr;
    VkDescriptorPool descriptor_pool_{};

    bool bCreated = false;
};


} // yu::vk