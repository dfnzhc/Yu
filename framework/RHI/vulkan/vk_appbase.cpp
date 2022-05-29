//
// Created by 秋鱼 on 2022/5/11.
//

#include "common/common.hpp"
#include "vk_appbase.hpp"
#include "vk_error.hpp"
#include "vk_utils.hpp"
#include "vk_initializers.hpp"
#include <vulkan/vulkan_win32.h>
#include <glfw_window.hpp>
#include <imgui.h>

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

    prepareVulkan();
    LOG_INFO("Successfully setup Vulkan, ready to render.")

    camera_.perspectiveInit(60.0, static_cast<float>(width_) / static_cast<float>(height_), 0.1f, 1000.0f);

    if (settings_.overlay) {
        ui_overlay_.device = vulkan_device_;
        ui_overlay_.queue = queue_;
        ui_overlay_.shaders = {
            loadShader("uioverlay.vert"),
            loadShader("uioverlay.frag"),
        };
        ui_overlay_.prepareResources();
        ui_overlay_.preparePipeline(pipeline_cache_, render_pass_);
    }
}

void AppBase::update(float delta_time)
{
    Application::update(delta_time);

    if (prepared) {
        draw();
        updateOverlay();

        updateUniformBuffers();
    }
}

void AppBase::finish()
{
    vkDeviceWaitIdle(device_);

    uniform_buffer_.destroy();

    vkDestroyDescriptorPool(device_, descriptor_pool_, nullptr);

    vkDestroyPipeline(device_, pipeline_, nullptr);
    vkDestroyPipelineLayout(device_, pipeline_layout_, nullptr);
    vkDestroyDescriptorSetLayout(device_, descriptor_set_layout_, nullptr);

    for (auto& module : shader_modules_) {
        vkDestroyShaderModule(device_, module.second, nullptr);
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

    if (settings_.overlay) {
        ui_overlay_.freeResources();
    }

    delete vulkan_device_;

    if (settings_.validation) {
        DestroyDebugMessenger(instance_, Debug_messenger_, nullptr);
    }

    vkDestroyInstance(instance_, nullptr);
}

bool AppBase::resize(const uint32_t width, const uint32_t height)
{
    LOG_INFO("Window size changed, rebuild some Vulkan infrastructures...")
    prepared = false;
    start_tracking_ = false;

    vkDeviceWaitIdle(device_);

    // 重新创建交换链
    width_ = width;
    height_ = height;
    createSwapChain();

    // 重新创建帧缓冲区
    vkDestroyImageView(device_, depthStencil.view, nullptr);
    vkDestroyImage(device_, depthStencil.image, nullptr);
    vkFreeMemory(device_, depthStencil.mem, nullptr);

    createDepthStencilView();
    for (auto& frameBuffer : frame_buffers_) {
        vkDestroyFramebuffer(device_, frameBuffer, nullptr);
    }
    setupFrameBuffer();

    if ((width > 0) && (height > 0)) {
        camera_.updateAspectRatio((float) width / (float) height);

        if (settings_.overlay) {
            ui_overlay_.resize(width, height);
        }
    }
    
    destroyCommandBuffers();
    createCommandBuffers();
    buildCommandBuffers();

    vkDeviceWaitIdle(device_);

    prepared = true;
    return true;
}

void AppBase::input_event(const InputEvent& input_event)
{
    Application::input_event(input_event);

    if (input_event.type == EventType::Keyboard) {
        [[maybe_unused]]
        const auto& key_event = static_cast<const KeyInputEvent&>(input_event);

        if (key_event.isEvent(Yuan::KeyCode::F1, KeyAction::Press)) {
            settings_.overlay = !settings_.overlay;
        }

//        LOG_INFO("[Keyboard] key: {}, action: {}", static_cast<int>(key_event.code), GetActionString(key_event.action));
    } else if (input_event.type == EventType::Mouse) {
        [[maybe_unused]]
        const auto& mouse_event = static_cast<const MouseInputEvent&>(input_event);

        if (!start_tracking_) {
            start_tracking_ = true;
            xPos_ = mouse_event.pos_x;
            yPos_ = mouse_event.pos_y;
        } else {
            float dx = mouse_event.pos_x - xPos_;
            float dy = mouse_event.pos_y - yPos_;
            xPos_ = mouse_event.pos_x;
            yPos_ = mouse_event.pos_y;

            if (mouse_event.isEvent(Yuan::MouseButton::Left, Yuan::MouseAction::PressedMove)) {
                camera_.update(dx, dy);
            }

            // 更新 Imgui 鼠标
            ImGuiIO& io = ImGui::GetIO();
            io.MousePos = ImVec2(xPos_, yPos_);
            io.MouseDown[0] = mouse_event.isEvent(Yuan::MouseButton::Left, Yuan::MouseAction::Press);
            io.MouseDown[1] = mouse_event.isEvent(Yuan::MouseButton::Right, Yuan::MouseAction::Press);
        }

//        LOG_INFO("[Mouse] key: {}, action: {} ({}.{} {})",
//                 GetButtonString(mouse_event.button),
//                 GetActionString(mouse_event.action), mouse_event.pos_x, mouse_event.pos_y, mouse_event.scroll_dir);
    }

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
    auto window = static_cast<GLFW_Window*>(platform_->getWindow());
    swap_chain_.initSurface(window->getGlfwWindowHandle());

    width_ = window->getExtent().width;
    height_ = window->getExtent().height;
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

void AppBase::prepareVulkan()
{
    LOG_INFO("Prepare vulkan infrastructures..")

    LOG_INFO("\tCreate swap chain...");
    createSwapChain();

    LOG_INFO("\tCreate command pool...");
    createCommandPool();

    LOG_INFO("\tCreate command buffers...");
    createCommandBuffers();

    LOG_INFO("\tCreate synchronization...");
    createSynchronizationPrimitives();

    LOG_INFO("\tCreate depth and stencil view...");
    createDepthStencilView();

    LOG_INFO("\tCreate render pass...");
    setupRenderPass();

    LOG_INFO("\tCreate framebuffer...");
    setupFrameBuffer();

    LOG_INFO("\tCreate uniform buffer...");
    setupUniformBuffers();

    LOG_INFO("\tCreate descriptor set layout...");
    setupDescriptorSetLayout();

    LOG_INFO("\tCreate pipeline cache...");
    createPipelineCache();

    LOG_INFO("\tcreate pipeline...");
    setupPipeline();

    LOG_INFO("\tCreate descriptor pool...");
    setupDescriptorPool();

    LOG_INFO("\tCreate descriptor set...");
    setupDescriptorSet();

    LOG_INFO("\tBuild command buffer...");
    buildCommandBuffers();

    prepared = true;
}

/**
 * @brief 首先根据窗口创建 surface，然后创建交换链，同时为交换链的图像创建图像视图
 */
void AppBase::createSwapChain()
{
    swap_chain_.create(&width_, &height_, settings_.vsync);
}

void AppBase::setupRenderPass()
{
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
    VkCommandPoolCreateInfo cmdPoolInfo = {};
    cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolInfo.queueFamilyIndex = swap_chain_.queueNodeIndex;
    cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CHECK(vkCreateCommandPool(device_, &cmdPoolInfo, nullptr, &cmd_pool_));
}

void AppBase::createCommandBuffers()
{
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
    // 只需要在具体实现的时候设置相应的命令缓冲区即可
    submit_info_ = ST::VK::submitInfo();
    submit_info_.pWaitDstStageMask = &submit_pipeline_stages_;
    submit_info_.waitSemaphoreCount = 1;
    submit_info_.pWaitSemaphores = &semaphores_.presentComplete;
    submit_info_.signalSemaphoreCount = 1;
    submit_info_.pSignalSemaphores = &semaphores_.renderComplete;

    // 创建交换链图像个数的 fence 用于同步
    VkFenceCreateInfo fenceCreateInfo = ST::VK::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    waitFences.resize(drawCmdBuffers.size());
    for (auto& fence : waitFences) {
        VK_CHECK(vkCreateFence(device_, &fenceCreateInfo, nullptr, &fence));
    }
}

void AppBase::createDepthStencilView()
{
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
    VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
    pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    VK_CHECK(vkCreatePipelineCache(device_, &pipelineCacheCreateInfo, nullptr, &pipeline_cache_));
}

void AppBase::setupFrameBuffer()
{
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

void AppBase::setupPipeline()
{
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
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(width_);
    viewport.height = static_cast<float>(height_);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = VkExtent2D{width_, height_};

    auto viewportState = ST::VK::pipelineViewportStateCreateInfo(1, 1);
    viewportState.pViewports = &viewport;
    viewportState.pScissors = &scissor;

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
    auto dynamicState = ST::VK::pipelineDynamicStateCreateInfo();
    dynamicState.pDynamicStates = dynamicStateEnables.data();
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());

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

void AppBase::setupUniformBuffers()
{
    VK_CHECK(vulkan_device_->createBuffer(
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &uniform_buffer_,
        2 * sizeof(glm::mat4)));

    updateUniformBuffers();
}

void AppBase::updateUniformBuffers()
{
    std::vector uniformData(2, glm::mat4{});
    uniformData[0] = camera_.matrices.view;
    uniformData[1] = camera_.matrices.perspective;

    VK_CHECK(uniform_buffer_.map());
    uniform_buffer_.copyTo(uniformData.data(), 2 * sizeof(glm::mat4));
    uniform_buffer_.unmap();
}

void AppBase::setupDescriptorSetLayout()
{
    // Binding 0: Uniform buffer (Vertex shader)
    auto layoutBinding = ST::VK::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0);
    auto descriptorLayout = ST::VK::descriptorSetLayoutCreateInfo(&layoutBinding, 1);
    VK_CHECK(vkCreateDescriptorSetLayout(device_, &descriptorLayout, nullptr, &descriptor_set_layout_));

    auto pPipelineLayoutCreateInfo = ST::VK::pipelineLayoutCreateInfo(nullptr, 0);
    pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pPipelineLayoutCreateInfo.pNext = nullptr;
    pPipelineLayoutCreateInfo.setLayoutCount = 1;
    pPipelineLayoutCreateInfo.pSetLayouts = &descriptor_set_layout_;

    VK_CHECK(vkCreatePipelineLayout(device_, &pPipelineLayoutCreateInfo, nullptr, &pipeline_layout_));
}

void AppBase::setupDescriptorPool()
{
    // 需要告知每种类型请求的描述符信息
    VkDescriptorPoolSize typeCounts[1];
    typeCounts[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    typeCounts[0].descriptorCount = 1;

    // 创建全局的描述符池
    VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
    descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolInfo.pNext = nullptr;
    descriptorPoolInfo.poolSizeCount = 1;
    descriptorPoolInfo.pPoolSizes = typeCounts;
    descriptorPoolInfo.maxSets = 1;

    VK_CHECK(vkCreateDescriptorPool(device_, &descriptorPoolInfo, nullptr, &descriptor_pool_));
}

void AppBase::setupDescriptorSet()
{
    // 从全局描述符池中分配一个新的描述符集
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptor_pool_;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptor_set_layout_;
    VK_CHECK(vkAllocateDescriptorSets(device_, &allocInfo, &descriptor_set_));

    // 更新决定着色器绑定点的描述符集
    // 对于着色器中使用的每个绑定点，都需要有一个描述符集与该绑定点相匹配

    VkWriteDescriptorSet writeDescriptorSet = {};
    // Binding 0 : Uniform buffer
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.dstSet = descriptor_set_;
    writeDescriptorSet.descriptorCount = 1;
    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeDescriptorSet.pBufferInfo = &uniform_buffer_.descriptor;
    // 将此 uniform 缓冲区绑定到绑定点0上
    writeDescriptorSet.dstBinding = 0;

    vkUpdateDescriptorSets(device_, 1, &writeDescriptorSet, 0, nullptr);
}

VkPipelineShaderStageCreateInfo AppBase::loadShader(std::string_view fileName)
{
    VkShaderModule shaderModule{};
    auto sd = shader_modules_.find(fileName.data());
    if (sd != shader_modules_.end()) {
        shaderModule = sd->second;
    } else {
        shaderModule = LoadShader(ST::GetSpvShaderFile(fileName.data()), device_);
        shader_modules_.insert({fileName.data(), shaderModule});
    }

    auto shaderStage = ST::VK::pipelineShaderStageCreateInfo();
    shaderStage.stage = GetShaderType(fileName);
    shaderStage.module = shaderModule;
    assert(shaderStage.module != VK_NULL_HANDLE);
    shaderStage.pName = "main";

    return shaderStage;
}

void AppBase::buildCommandBuffers()
{
    auto cmdBeginInfo = ST::VK::commandBufferBeginInfo();

    VkClearValue clearValues[2];
    clearValues[0].color = {{0.2f, 0.3f, 0.7f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    auto renderPassBeginInfo = ST::VK::renderPassBeginInfo();
    renderPassBeginInfo.renderPass = render_pass_;
    renderPassBeginInfo.renderArea.offset = {0, 0};
    renderPassBeginInfo.renderArea.extent = {width_, height_};
    renderPassBeginInfo.clearValueCount = 2;
    renderPassBeginInfo.pClearValues = clearValues;

    const VkViewport viewport = ST::VK::viewport((float) width_, (float) height_, 0.0f, 1.0f);
    const VkRect2D scissor = ST::VK::rect2D(width_, height_, 0, 0);

    for (auto i = 0; i < drawCmdBuffers.size(); ++i) {
        renderPassBeginInfo.framebuffer = frame_buffers_[i];

        auto cmdBuffer = drawCmdBuffers[i];
        VK_CHECK(vkBeginCommandBuffer(cmdBuffer, &cmdBeginInfo));
        vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        // 动态更新视口
        vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

        // 动态更新裁剪矩形
        vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

        // 绑定描述符集，描述着色器的绑定点
        vkCmdBindDescriptorSets(
            cmdBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline_layout_,
            0,
            1,
            &descriptor_set_,
            0,
            nullptr);

        vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
        vkCmdDraw(cmdBuffer, 3, 1, 0, 0);
        drawUI(cmdBuffer);
        vkCmdEndRenderPass(cmdBuffer);

        VK_CHECK(vkEndCommandBuffer(cmdBuffer));
    }
}

void AppBase::draw()
{
    // 获取交换链下一个图像的索引
    VK_CHECK(swap_chain_.acquireNextImage(semaphores_.presentComplete, &current_buffer_));

    // 使用 fence 同步 CPU 与 GPU，直到该命令缓冲区中的命令被执行完毕
    VK_CHECK(vkWaitForFences(device_, 1, &waitFences[current_buffer_], VK_TRUE, UINT64_MAX));
    VK_CHECK(vkResetFences(device_, 1, &waitFences[current_buffer_]));

    // 设置提交信息
    submit_info_.commandBufferCount = 1;
    submit_info_.pCommandBuffers = &drawCmdBuffers[current_buffer_];

    // 将命令缓冲区提交到图像队列，并传入相应的 fence 用于同步
    VK_CHECK(vkQueueSubmit(queue_, 1, &submit_info_, waitFences[current_buffer_]));

    // 将当前的缓冲区呈现给交换链
    // 将 submitInfo 中命令缓冲区提交所发出的信号传递给交换链作为等待信号
    // 这样可以确保在图像被呈现给窗口系统之前，所有的命令都已经被提交
    VkResult present = swap_chain_.queuePresent(queue_, current_buffer_, semaphores_.renderComplete);
    if (!((present == VK_SUCCESS) || (present == VK_SUBOPTIMAL_KHR))) {
        VK_CHECK(present);
    }
}

void AppBase::drawUI(const VkCommandBuffer commandBuffer)
{
    if (settings_.overlay) {
        const VkViewport viewport = ST::VK::viewport((float) width_, (float) height_, 0.0f, 1.0f);
        const VkRect2D scissor = ST::VK::rect2D(width_, height_, 0, 0);
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        ui_overlay_.draw(commandBuffer);
    }
}

void AppBase::updateOverlay()
{
    if (!settings_.overlay)
        return;

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float) width_, (float) height_);
    io.DeltaTime = 1.0f / 30.0f; //frameTimer;

    ImGui::NewFrame();

    ImGui::Begin("Hello", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    ImGui::TextUnformatted(name_.c_str());
    ImGui::TextUnformatted(device_properties_.deviceName);
//    updateUIOverlay(&ui_overlay_);
    if (ui_overlay_.header("Test")) {

        ImGui::NewLine();
    }
    ImGui::End();

    ImGui::Render();

    if (ui_overlay_.update() || ui_overlay_.updated) {
        buildCommandBuffers();
        ui_overlay_.updated = false;
    }
}

} // namespace ST