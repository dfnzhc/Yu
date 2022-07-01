//
// Created by 秋鱼 on 2022/6/25.
//

#include "imgui.hpp"
#include "error.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include <common/common.hpp>
#include <common/imgui_impl_glfw.h>

#include "initializers.hpp"
#include "imgui_impl_vulkan.h"

namespace yu::vk {

constexpr float FontSize = 15.f;

ImGUI::ImGUI()
{
    // Init ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    // Color scheme
    ImGuiStyle& style = ImGui::GetStyle();
//    style.Colors[ImGuiCol_TitleBg] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
//    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
//    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.0f, 0.0f, 0.0f, 0.1f);
//    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
//    style.Colors[ImGuiCol_Header] = ImVec4(0.8f, 0.0f, 0.0f, 0.4f);
//    style.Colors[ImGuiCol_HeaderActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
//    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
//    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.8f);
//    style.Colors[ImGuiCol_CheckMark] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
//    style.Colors[ImGuiCol_SliderGrab] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
//    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
//    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(1.0f, 1.0f, 1.0f, 0.1f);
//    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(1.0f, 1.0f, 1.0f, 0.2f);
//    style.Colors[ImGuiCol_Button] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
//    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
//    style.Colors[ImGuiCol_ButtonActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);

    // 获取 UI 的图片
    ImGuiIO& io = ImGui::GetIO();
    (void) io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
//    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // 设置 ui 的字体
    const auto fontPath = AssetPath() + "/HarmonyOS_Sans_Medium.ttf";
    io.Fonts->AddFontFromFileTTF(fontPath.c_str(), FontSize, nullptr, io.Fonts->GetGlyphRangesChineseFull());
}

ImGUI::~ImGUI()
{
}

void ImGUI::create(const VulkanInstance& instance,
                   const VulkanDevice& device,
                   VkRenderPass renderPass,
                   UploadHeap& uploadHeap,
                   GLFWwindow* window)
{
    device_ = const_cast<VulkanDevice*>(&device);
    instance_ = &instance;

    std::vector<VkDescriptorPoolSize> poolSizes = {
        {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}
    };

    VkDescriptorPoolCreateInfo
        poolInfo = descriptorPoolCreateInfo(poolSizes, 1000 * static_cast<uint32_t>(poolSizes.size()));
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    VK_CHECK(vkCreateDescriptorPool(device.getHandle(), &poolInfo, nullptr, &descriptor_pool_));

    ImGui_ImplGlfw_InitForVulkan(window, true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = instance.getHandle();
    init_info.PhysicalDevice = device.getProperties().physical_device;
    init_info.Device = device.getHandle();
    init_info.Queue = device.getGraphicsQueue();
    init_info.DescriptorPool = descriptor_pool_;
    init_info.Subpass = 0;
    init_info.MinImageCount = 3;
    init_info.ImageCount = 3;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.PipelineCache = device.getPipelineCache();
    ImGui_ImplVulkan_Init(&init_info, renderPass);

    auto copyCmd = device_->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
//    ImGui_ImplVulkan_CreateFontsTexture(uploadHeap.getCommandBuffer());
//    uploadHeap.flushAndFinish();
    ImGui_ImplVulkan_CreateFontsTexture(copyCmd);
    device_->flushCommandBuffer(copyCmd, device.getGraphicsQueue(), true);

    //clear font textures from cpu data
    ImGui_ImplVulkan_DestroyFontUploadObjects();

    bCreated = true;
}

void ImGUI::destroy()
{
    if (bCreated) {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
    }

    if (ImGui::GetCurrentContext()) {
        ImGui::DestroyContext();
    }

    if (descriptor_pool_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device_->getHandle(), descriptor_pool_, nullptr);
        descriptor_pool_ = VK_NULL_HANDLE;
    }
}

void ImGUI::draw(VkCommandBuffer cmdBuffer)
{
    ImDrawData* draw_data = ImGui::GetDrawData();
    ImGui_ImplVulkan_RenderDrawData(draw_data, cmdBuffer);
}

} // yu::vk
