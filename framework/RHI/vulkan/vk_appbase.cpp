﻿//
// Created by 秋鱼 on 2022/5/11.
//

#include "common/common.hpp"
#include "vk_appbase.hpp"
#include "vk_error.hpp"
#include "vk_utils.hpp"
#include "vk_initializers.hpp"
#include <vulkan/vulkan_win32.h>
#include <glfw_window.hpp>

namespace ST::VK {

bool AppBase::prepare(Platform& platform)
{
    if (!Application::prepare(platform)) {
        return false;
    }

    return true;
}

void AppBase::setup()
{
    Application::setup();

    initVulkan();
    prepare();
}

void AppBase::update(float delta_time)
{
    Application::update(delta_time);
}

void AppBase::finish()
{
    vkDeviceWaitIdle(device_);

    vkDestroyPipeline(device_, pipeline_, nullptr);
    vkDestroyPipelineLayout(device_, pipeline_layout_, nullptr);

    for (auto& module : shader_modules_) {
        vkDestroyShaderModule(device_, module, nullptr);
    }

    for (auto& frameBuffer : frame_buffers_) {
        vkDestroyFramebuffer(device_, frameBuffer, nullptr);
    }

    vkDestroyPipelineCache(device_, pipeline_cache_, nullptr);

    if (render_pass_ != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device_, render_pass_, nullptr);
    }

    vkDestroyImageView(device_, depthStencil.view, nullptr);
    vkDestroyImage(device_, depthStencil.image, nullptr);
    vkFreeMemory(device_, depthStencil.mem, nullptr);

    vkDestroySemaphore(device_, semaphores_.presentComplete, nullptr);
    vkDestroySemaphore(device_, semaphores_.renderComplete, nullptr);
    for (auto& fence : waitFences) {
        vkDestroyFence(device_, fence, nullptr);
    }

    destroyCommandBuffers();
    vkDestroyCommandPool(device_, cmd_pool_, nullptr);

    swap_chain_.cleanup();

    delete vulkan_device_;

    if (settings_.validation) {
        DestroyDebugMessenger(instance_, Debug_messenger_, nullptr);
    }

    vkDestroyInstance(instance_, nullptr);
}

bool AppBase::resize(const uint32_t width, const uint32_t height)
{

    return true;
}

void AppBase::input_event(const InputEvent& input_event)
{
    Application::input_event(input_event);
}

/**
 * @brief 按以下的流程初始化 Vulkan
 * 
 * 创建 Vulkan 实例：Instance、DebugMessenger
 * 创建设备：物理设备和逻辑设备
 * 创建 surface 和交换链
 * 创建用于同步的互斥量
 * 
 */
void AppBase::initVulkan()
{
    LOG_INFO("Init vulkan...")

    // set the extensions
    setInstanceExtensions();
    setDeviceExtensions();

    // first create vulkan instance
    createInstance();

    // physical device
    LOG_INFO("\tEnumerate physical devices...");
    uint32_t gpuCount = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(instance_, &gpuCount, nullptr));
    if (gpuCount == 0) {
        throw std::runtime_error("No device with Vulkan support found");
    }
    // Enumerate devices
    std::vector<VkPhysicalDevice> physicalDevices(gpuCount);
    VK_CHECK(vkEnumeratePhysicalDevices(instance_, &gpuCount, physicalDevices.data()));

    const int gpuIdx = 0;
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevices[gpuIdx], &deviceProperties);
    LOG_INFO("\t\tSelected device: {}", deviceProperties.deviceName);
    LOG_INFO("\t\tTYPE: {}", PhysicalDeviceTypeString(deviceProperties.deviceType));
    LOG_INFO("\t\tAPI: {}.{}.{}",
             (deviceProperties.apiVersion >> 22),
             ((deviceProperties.apiVersion >> 12) & 0x3ff),
             (deviceProperties.apiVersion & 0xfff));

    physical_device_ = physicalDevices[gpuIdx];
    // 存储物理设备的属性（包括限制）、特征和内存属性（以便实例可以对照它们进行检查）
    vkGetPhysicalDeviceProperties(physical_device_, &deviceProperties);
    vkGetPhysicalDeviceFeatures(physical_device_, &device_features_);
    vkGetPhysicalDeviceMemoryProperties(physical_device_, &device_memory_properties_);

    getDeviceEnabledFeatures();

    // 创建Vulkan设备
    // 这由一个单独的类来处理，该类得到一个逻辑设备的表示，并封装了与设备有关的功能
    LOG_INFO("\tCreate vulkan devices...");
    vulkan_device_ = new VulkanDevice(physical_device_);
    VK_CHECK(vulkan_device_->createLogicalDevice(enabled_features_, enabled_device_extensions_, device_create_pNext));

    device_ = vulkan_device_->logicalDevice;

    // 从设备上获取图形队列
    vkGetDeviceQueue(device_, vulkan_device_->queueFamilyIndices.graphics, 0, &queue_);

    // 寻找一个合适的深度格式
    VkBool32 validDepthFormat = GetSupportedDepthFormat(physical_device_, &depth_format_);
    assert(validDepthFormat);

    // 向交换链设置相关的属性
    swap_chain_.connect(instance_, physical_device_, device_);

    // 创建同步对象
    VkSemaphoreCreateInfo semaphoreCreateInfo = ST::VK::semaphoreCreateInfo();
    // 创建一个用于同步图像显示的信号
    // 确保在开始向队列提交新的命令之前显示图像
    VK_CHECK(vkCreateSemaphore(device_, &semaphoreCreateInfo, nullptr, &semaphores_.presentComplete));
    // 创建一个用于同步命令提交的信号
    // 确保在所有的命令被提交和执行之前，图像不会被呈现
    VK_CHECK(vkCreateSemaphore(device_, &semaphoreCreateInfo, nullptr, &semaphores_.renderComplete));

    // 设置提交信息结构
    // 信号量在应用程序生命周期内保持不变
    // 提交时等待交换链显示完成，提交后提示能够渲染
    // 命令缓冲区的提交信息是在具体实现的时候设置的
    submit_info_ = ST::VK::submitInfo();
    submit_info_.pWaitDstStageMask = &submit_pipeline_stages_;
    submit_info_.waitSemaphoreCount = 1;
    submit_info_.pWaitSemaphores = &semaphores_.presentComplete;
    submit_info_.signalSemaphoreCount = 1;
    submit_info_.pSignalSemaphores = &semaphores_.renderComplete;
}

void AppBase::createInstance()
{
    LOG_INFO("\tCreate vulkan instance...");
    VkApplicationInfo appInfo{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = name_.c_str(),
        .pEngineName = "Setaria",
        .apiVersion = vulkan_properties_.apiVersion
    };

    std::vector<const char*> instanceExtensions = {VK_KHR_SURFACE_EXTENSION_NAME};

#if defined(_WIN32)
    instanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#else
    throw std::runtime_error("Only support windows so far.")
#endif

    uint32_t extCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
    if (extCount > 0) {
        std::vector<VkExtensionProperties> extensions(extCount);
        if (VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &extCount, extensions.data()))) {
            for (VkExtensionProperties ext : extensions) {
                supported_instance_extensions_.emplace_back(ext.extensionName);
            }
        }
    }

    if (!enabled_instance_extensions_.empty()) {
        for (const char* ext : enabled_instance_extensions_) {
            if (std::find(supported_instance_extensions_.begin(), supported_instance_extensions_.end(), ext)
                == supported_instance_extensions_.end()) {
                throw std::runtime_error("The extension " + std::string{ext} + " is not support at instance level.");
            }
            instanceExtensions.push_back(ext);
        }
    }

    VkInstanceCreateInfo instanceCreateInfo{};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;
    instanceCreateInfo.pNext = nullptr;

    if (!instanceExtensions.empty()) {
        if (settings_.validation) {
            instanceExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
        instanceCreateInfo.enabledExtensionCount = (uint32_t) instanceExtensions.size();
        instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();
    }

    const char* validationLayerName = "VK_LAYER_KHRONOS_validation";
    if (settings_.validation) {
        // Check if this layer is available at instance level
        uint32_t instanceLayerCount;
        vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
        std::vector<VkLayerProperties> instanceLayerProperties(instanceLayerCount);
        vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayerProperties.data());
        bool validationLayerPresent = false;
        for (VkLayerProperties layer : instanceLayerProperties) {
            if (strcmp(layer.layerName, validationLayerName) == 0) {
                validationLayerPresent = true;
                break;
            }
        }
        if (validationLayerPresent) {
            instanceCreateInfo.ppEnabledLayerNames = &validationLayerName;
            instanceCreateInfo.enabledLayerCount = 1;
        } else {
            throw std::runtime_error("Validation layer VK_LAYER_KHRONOS_validation not present, validation is disabled");
        }
    }

    VK_CHECK(vkCreateInstance(&instanceCreateInfo, nullptr, &instance_));

    if (settings_.validation) {
        SetupDebugMessenger(instance_, nullptr, &Debug_messenger_);
    }

}

void AppBase::prepare()
{
    createSwapChain();

    createCommandPool();
    createCommandBuffers();
    createSynchronizationPrimitives();
    createDepthStencilView();
    setupRenderPass();
    setupFrameBuffer();

    createPipelineCache();
    preparePipeline();
}

/**
 * @brief 首先根据窗口创建 surface，然后创建交换链，同时为交换链的图像创建图像视图
 */
void AppBase::createSwapChain()
{
    auto window = static_cast<GLFW_Window*>(platform_->getWindow());

    swap_chain_.initSurface(window->getGlfwWindowHandle());

    width_ = window->getExtent().width;
    height_ = window->getExtent().height;
    swap_chain_.create(&width_, &height_, settings_.vsync);
}

void AppBase::setupRenderPass()
{
    LOG_INFO("\tCreate render pass...");
    std::array<VkAttachmentDescription, 2> attachments = {};
    // Color attachment
    attachments[0].format = swap_chain_.colorFormat;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    // Depth attachment
    attachments[1].format = depth_format_;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorReference = {};
    colorReference.attachment = 0;
    colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthReference = {};
    depthReference.attachment = 1;
    depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDescription = {};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colorReference;
    subpassDescription.pDepthStencilAttachment = &depthReference;
    subpassDescription.inputAttachmentCount = 0;
    subpassDescription.pInputAttachments = nullptr;
    subpassDescription.preserveAttachmentCount = 0;
    subpassDescription.pPreserveAttachments = nullptr;
    subpassDescription.pResolveAttachments = nullptr;

    // Subpass dependencies for layout transitions
    std::array<VkSubpassDependency, 2> dependencies;

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpassDescription;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();

    VK_CHECK(vkCreateRenderPass(device_, &renderPassInfo, nullptr, &render_pass_));
}

void AppBase::createCommandPool()
{
    LOG_INFO("\tCreate command pool...");
    VkCommandPoolCreateInfo cmdPoolInfo = {};
    cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolInfo.queueFamilyIndex = swap_chain_.queueNodeIndex;
    cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CHECK(vkCreateCommandPool(device_, &cmdPoolInfo, nullptr, &cmd_pool_));
}

void AppBase::createCommandBuffers()
{
    LOG_INFO("\tCreate command buffers...");
    // 为每个交换链图像创建一个命令缓冲区，并重复使用以进行渲染
    drawCmdBuffers.resize(swap_chain_.imageCount);

    VkCommandBufferAllocateInfo cmdBufAllocateInfo =
        ST::VK::commandBufferAllocateInfo(
            cmd_pool_,
            VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            static_cast<uint32_t>(drawCmdBuffers.size()));

    VK_CHECK(vkAllocateCommandBuffers(device_, &cmdBufAllocateInfo, drawCmdBuffers.data()));
}

void AppBase::destroyCommandBuffers()
{
    vkFreeCommandBuffers(device_, cmd_pool_, static_cast<uint32_t>(drawCmdBuffers.size()), drawCmdBuffers.data());
}

void AppBase::createSynchronizationPrimitives()
{
    LOG_INFO("\tCreate synchronization...");
    // Wait fences to sync command buffer access
    VkFenceCreateInfo fenceCreateInfo = ST::VK::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    waitFences.resize(drawCmdBuffers.size());
    for (auto& fence : waitFences) {
        VK_CHECK(vkCreateFence(device_, &fenceCreateInfo, nullptr, &fence));
    }
}

void AppBase::createDepthStencilView()
{
    LOG_INFO("\tCreate depth and stencil view...");
    VkImageCreateInfo imageCI{};
    imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCI.imageType = VK_IMAGE_TYPE_2D;
    imageCI.format = depth_format_;
    imageCI.extent = {width_, height_, 1};
    imageCI.mipLevels = 1;
    imageCI.arrayLayers = 1;
    imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VK_CHECK(vkCreateImage(device_, &imageCI, nullptr, &depthStencil.image));

    VkMemoryRequirements memReqs{};
    vkGetImageMemoryRequirements(device_, depthStencil.image, &memReqs);

    VkMemoryAllocateInfo memAllloc{};
    memAllloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAllloc.allocationSize = memReqs.size;
    memAllloc.memoryTypeIndex = vulkan_device_->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VK_CHECK(vkAllocateMemory(device_, &memAllloc, nullptr, &depthStencil.mem));
    VK_CHECK(vkBindImageMemory(device_, depthStencil.image, depthStencil.mem, 0));

    VkImageViewCreateInfo imageViewCI{};
    imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCI.image = depthStencil.image;
    imageViewCI.format = depth_format_;
    imageViewCI.subresourceRange.baseMipLevel = 0;
    imageViewCI.subresourceRange.levelCount = 1;
    imageViewCI.subresourceRange.baseArrayLayer = 0;
    imageViewCI.subresourceRange.layerCount = 1;
    imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    // Stencil aspect should only be set on depth + stencil formats (VK_FORMAT_D16_UNORM_S8_UINT..VK_FORMAT_D32_SFLOAT_S8_UINT
    if (depth_format_ >= VK_FORMAT_D16_UNORM_S8_UINT) {
        imageViewCI.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    VK_CHECK(vkCreateImageView(device_, &imageViewCI, nullptr, &depthStencil.view));
}

void AppBase::createPipelineCache()
{
    LOG_INFO("\tCreate pipeline cache...");
    VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
    pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    VK_CHECK(vkCreatePipelineCache(device_, &pipelineCacheCreateInfo, nullptr, &pipeline_cache_));
}

void AppBase::setupFrameBuffer()
{
    LOG_INFO("\tCreate framebuffer...");
    VkImageView attachments[2];

    // 深度/模板的 attachment 对所有帧缓冲区都是一样的
    attachments[1] = depthStencil.view;

    VkFramebufferCreateInfo frameBufferCreateInfo = {};
    frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frameBufferCreateInfo.pNext = nullptr;
    frameBufferCreateInfo.renderPass = render_pass_;
    frameBufferCreateInfo.attachmentCount = 2;
    frameBufferCreateInfo.pAttachments = attachments;
    frameBufferCreateInfo.width = width_;
    frameBufferCreateInfo.height = height_;
    frameBufferCreateInfo.layers = 1;

    // 为每个交换链图像创建帧缓冲器
    frame_buffers_.resize(swap_chain_.imageCount);
    for (uint32_t i = 0; i < frame_buffers_.size(); i++) {
        attachments[0] = swap_chain_.buffers[i].view;
        VK_CHECK(vkCreateFramebuffer(device_, &frameBufferCreateInfo, nullptr, &frame_buffers_[i]));
    }
}

void AppBase::preparePipeline()
{
    LOG_INFO("\tcreate pipeline...");
    auto pipelineCI = ST::VK::pipelineCreateInfo();

    LOG_INFO("\t\tSet shader modules..");
    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};

    // Vertex shader
    shaderStages[0] = loadShader("01_shader_base.vert");
    // Fragment shader
    shaderStages[1] = loadShader("01_shader_base.frag");

    LOG_INFO("\t\tSet vertex input..");
    auto vertexInputState = ST::VK::pipelineVertexInputStateCreateInfo();
    vertexInputState.vertexBindingDescriptionCount = 0;
    vertexInputState.pVertexBindingDescriptions = nullptr; // Optional
    vertexInputState.vertexAttributeDescriptionCount = 0;
    vertexInputState.pVertexAttributeDescriptions = nullptr; // Optional

    LOG_INFO("\t\tSet input assembly state..");
    auto inputAssemblyState = ST::VK::pipelineInputAssemblyStateCreateInfo();
    inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    LOG_INFO("\t\tSet Rasterization state..");
    auto rasterizationState = ST::VK::pipelineRasterizationStateCreateInfo();
    rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationState.cullMode = VK_CULL_MODE_NONE;
    rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationState.rasterizerDiscardEnable = VK_FALSE;
    rasterizationState.depthBiasEnable = VK_FALSE;

    LOG_INFO("\t\tSet color blend state..");
    auto blendAttachmentState = ST::VK::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
    auto colorBlendState = ST::VK::pipelineColorBlendStateCreateInfo();
    colorBlendState.attachmentCount = 1;
    colorBlendState.pAttachments = &blendAttachmentState;

    LOG_INFO("\t\tSet sample state...");
    auto multisampleState = ST::VK::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);

    LOG_INFO("\t\tSet viewport state...");
    auto viewportState = ST::VK::pipelineViewportStateCreateInfo(1, 1);

    LOG_INFO("\t\tSet depth stencil state...");
    auto depthStencilState = ST::VK::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE);
    depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilState.depthBoundsTestEnable = VK_FALSE;
    depthStencilState.back.failOp = VK_STENCIL_OP_KEEP;
    depthStencilState.back.passOp = VK_STENCIL_OP_KEEP;
    depthStencilState.stencilTestEnable = VK_FALSE;
    depthStencilState.front = depthStencilState.back;

    LOG_INFO("\t\tSet pipeline dynamic state...");
    std::vector<VkDynamicState> dynamicStateEnables;
    dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);
    dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);
    VkPipelineDynamicStateCreateInfo
        dynamicState = ST::VK::pipelineDynamicStateCreateInfo(dynamicStateEnables.data(), static_cast<uint32_t>(dynamicStateEnables.size()));

    LOG_INFO("\t\tSet pipeline layout...");
    setupDescriptorSetLayout();

    pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineCI.pStages = shaderStages.data();
    pipelineCI.pVertexInputState = &vertexInputState;
    pipelineCI.pInputAssemblyState = &inputAssemblyState;
    pipelineCI.pRasterizationState = &rasterizationState;
    pipelineCI.pColorBlendState = &colorBlendState;
    pipelineCI.pMultisampleState = &multisampleState;
    pipelineCI.pViewportState = &viewportState;
    pipelineCI.pDepthStencilState = &depthStencilState;
    pipelineCI.pDynamicState = &dynamicState;
    pipelineCI.layout = pipeline_layout_;
    pipelineCI.renderPass = render_pass_;
    pipelineCI.subpass = 0;
    pipelineCI.basePipelineHandle = VK_NULL_HANDLE;

    VK_CHECK(vkCreateGraphicsPipelines(device_, pipeline_cache_, 1, &pipelineCI, nullptr, &pipeline_));
}

void AppBase::setupDescriptorSetLayout()
{
    auto pPipelineLayoutCreateInfo = ST::VK::pipelineLayoutCreateInfo(nullptr, 0);
    pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pPipelineLayoutCreateInfo.pNext = nullptr;

    VK_CHECK(vkCreatePipelineLayout(device_, &pPipelineLayoutCreateInfo, nullptr, &pipeline_layout_));
}

VkPipelineShaderStageCreateInfo AppBase::loadShader(std::string_view fileName)
{
    auto shaderStage = ST::VK::pipelineShaderStageCreateInfo();
    shaderStage.stage = GetShaderType(fileName);
    shaderStage.module = LoadShader(ST::GetSpvShaderFile(fileName.data()), device_);
    assert(shaderStage.module != VK_NULL_HANDLE);
    shaderStage.pName = "main";

    shader_modules_.push_back(shaderStage.module);
    return shaderStage;
}

} // namespace ST