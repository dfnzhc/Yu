//
// Created by 秋鱼 on 2022/6/25.
//

#include "imgui.hpp"
#include "error.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include <common/common.hpp>

#include "initializers.hpp"

namespace yu::vk {

constexpr float FontSize = 15.f;

ImGUI::ImGUI()
{
    // Init ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    // Color scheme
    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_TitleBg] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.0f, 0.0f, 0.0f, 0.1f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.8f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.8f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(1.0f, 1.0f, 1.0f, 0.1f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(1.0f, 1.0f, 1.0f, 0.2f);
    style.Colors[ImGuiCol_Button] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);

    ImGuiIO& io = ImGui::GetIO();
    (void) io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
    io.ConfigViewportsNoAutoMerge = true;
    io.ConfigViewportsNoTaskBarIcon = true;

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // 设置 ui 的字体
    const auto fontPath = AssetPath() + "/HarmonyOS_Sans_Medium.ttf";
    io.Fonts->AddFontFromFileTTF(fontPath.c_str(),
                                 FontSize,
                                 nullptr,
                                 io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
    io.Fonts->Flags |= ImFontAtlasFlags_NoPowerOfTwoHeight;
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
}

ImGUI::~ImGUI()
{
    if (ImGui::GetCurrentContext()) {
        ImGui::DestroyContext();
    }
}

struct VERTEX_CONSTANT_BUFFER
{
    float mvp[4][4];
};

void ImGUI::create(const VulkanDevice& device,
                   const DynamicBuffer& constantBuffer,
                   VkRenderPass renderPass,
                   UploadHeap& uploadHeap)
{
    device_ = &device;
    constant_buffer_ = const_cast<DynamicBuffer*>(&constantBuffer);

    // 获取 imGui 的字体图片
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    uint32_t uploadSize = width * height * 4 * sizeof(uint8_t);

    // 创建字体图片
    {
        auto imageInfo = imageCreateInfo();
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VK_CHECK(vkCreateImage(device.getHandle(), &imageInfo, nullptr, &font_texture_));
        VkMemoryRequirements memReqs;
        vkGetImageMemoryRequirements(device.getHandle(), font_texture_, &memReqs);
        VkMemoryAllocateInfo memAllocInfo = memoryAllocateInfo();
        memAllocInfo.allocationSize = memReqs.size;
        bool pass = device.getProperties().getMemoryType(memReqs.memoryTypeBits,
                                                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                                         &memAllocInfo.memoryTypeIndex);
        assert(pass && "No mappable, coherent memory");
        VK_CHECK(vkAllocateMemory(device.getHandle(), &memAllocInfo, nullptr, &font_memory_));
        VK_CHECK(vkBindImageMemory(device.getHandle(), font_texture_, font_memory_, 0));

//        VmaAllocationCreateInfo imageAllocCreateInfo = {};
//        imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
//        imageAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
//        imageAllocCreateInfo.pUserData = (void*) ("ImGUI tex");
//        VmaAllocationInfo gpuImageAllocInfo = {};
//        VK_CHECK(vmaCreateImage(device.getAllocator(),
//                                &imageInfo,
//                                &imageAllocCreateInfo,
//                                &font_texture_,
//                                &tex_allocation_,
//                                &gpuImageAllocInfo));
    }

    // 创建图片视图和采样器
    {
        CreateImageView(device.getHandle(),
                        font_texture_,
                        VK_FORMAT_R8G8B8A8_UNORM,
                        VK_IMAGE_ASPECT_COLOR_BIT,
                        texture_view_);

        VkSamplerCreateInfo samplerInfo = {};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        VK_CHECK(vkCreateSampler(device.getHandle(), &samplerInfo, nullptr, &sampler_));
    }

    // 上传 ImGui 的图片视图到设备中
    // FIXME：图片的上传有问题
    auto* ptr = uploadHeap.alloc(uploadSize, 512);
    std::memcpy(ptr, pixels, uploadSize);

    // 前置 barrier
    {
        auto copyBarrier = imageMemoryBarrier();
        copyBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        copyBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        copyBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        copyBarrier.image = font_texture_;
        copyBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyBarrier.subresourceRange.levelCount = 1;
        copyBarrier.subresourceRange.layerCount = 1;
        vkCmdPipelineBarrier(uploadHeap.getCommandBuffer(),
                             VK_PIPELINE_STAGE_HOST_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0,
                             0,
                             nullptr,
                             0,
                             nullptr,
                             1,
                             &copyBarrier);

        VkBufferImageCopy region = {};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.layerCount = 1;
        region.imageExtent.width = width;
        region.imageExtent.height = height;
        region.imageExtent.depth = 1;
        vkCmdCopyBufferToImage(uploadHeap.getCommandBuffer(),
                               uploadHeap.getBuffer(),
                               font_texture_,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1,
                               &region);

        auto useBarrier = imageMemoryBarrier();
        useBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        useBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        useBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        useBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        useBarrier.image = font_texture_;
        useBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        useBarrier.subresourceRange.levelCount = 1;
        useBarrier.subresourceRange.layerCount = 1;
        vkCmdPipelineBarrier(uploadHeap.getCommandBuffer(),
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0,
                             0,
                             nullptr,
                             0,
                             nullptr,
                             1,
                             &useBarrier);
    }
    uploadHeap.flushAndFinish();

    // 创建描述符相关变量
    // Descriptor pool
    std::vector<VkDescriptorPoolSize> poolSizes = {
        descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, DESCRIPTOR_COUNT),
    };

    auto descriptorPoolInfo = descriptorPoolCreateInfo(poolSizes, 2 * DESCRIPTOR_COUNT);
    VK_CHECK(vkCreateDescriptorPool(device.getHandle(), &descriptorPoolInfo, nullptr, &descriptor_pool_));

    // Descriptor set layout
    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
        descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                   VK_SHADER_STAGE_FRAGMENT_BIT,
                                   0)
    };
    auto descriptorLayout = descriptorSetLayoutCreateInfo(setLayoutBindings);
    VK_CHECK(vkCreateDescriptorSetLayout(device.getHandle(),
                                         &descriptorLayout,
                                         nullptr,
                                         &descriptor_set_layout_));

    // 创建描述符并设置、更新绑定资源信息
    auto allocInfo = descriptorSetAllocateInfo(descriptor_pool_, &descriptor_set_layout_, 1);
    auto fontDescriptor = descriptorImageInfo(
        sampler_,
        texture_view_,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );

    descriptor_set_.resize(DESCRIPTOR_COUNT);
    for (int i = 0; i < static_cast<int>(DESCRIPTOR_COUNT); i++) {
        VK_CHECK(vkAllocateDescriptorSets(device.getHandle(), &allocInfo, &descriptor_set_[i]));
        auto write = writeDescriptorSet(descriptor_set_[i],
                                        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                        0,
                                        &fontDescriptor);

        vkUpdateDescriptorSets(device.getHandle(),
                               1,
                               &write,
                               0,
                               nullptr);
    }

    // 设置流水线相关
    auto layoutInfo = pipelineLayoutCreateInfo(&descriptor_set_layout_, 1);
    auto pushConstant = pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(PushConstBlock), 0);
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushConstant;

    VK_CHECK(vkCreatePipelineLayout(device.getHandle(), &layoutInfo, nullptr, &pipeline_layout_));

    pipeline_builder_.create(device);
    pipeline_builder_.setShader({"_imgui.vert", "_imgui.frag"});
    updatePipeline(renderPass);
}

void ImGUI::destroy()
{
    if (descriptor_pool_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device_->getHandle(), descriptor_pool_, nullptr);
        descriptor_pool_ = VK_NULL_HANDLE;
    }

    pipeline_builder_.destroy();
    vkDestroyPipeline(device_->getHandle(), pipeline_, nullptr);
    pipeline_ = VK_NULL_HANDLE;
    vkDestroyPipelineLayout(device_->getHandle(), pipeline_layout_, nullptr);
    pipeline_layout_ = VK_NULL_HANDLE;

    vkDestroyDescriptorSetLayout(device_->getHandle(), descriptor_set_layout_, nullptr);
    descriptor_set_layout_ = VK_NULL_HANDLE;
    vkDestroyDescriptorPool(device_->getHandle(), descriptor_pool_, nullptr);
    descriptor_pool_ = VK_NULL_HANDLE;

//    if (font_texture_ != VK_NULL_HANDLE) {
//        vmaDestroyImage(device_->getAllocator(), font_texture_, tex_allocation_);
//        font_texture_ = VK_NULL_HANDLE;
//    }

    if (font_texture_ != VK_NULL_HANDLE) {
        vkDestroyImage(device_->getHandle(), font_texture_, nullptr);
        font_texture_ = VK_NULL_HANDLE;
    }
    vkFreeMemory(device_->getHandle(), font_memory_, nullptr);
    font_memory_ = VK_NULL_HANDLE;

    vkDestroyImageView(device_->getHandle(), texture_view_, nullptr);
    texture_view_ = VK_NULL_HANDLE;
    vkDestroySampler(device_->getHandle(), sampler_, nullptr);
    sampler_ = VK_NULL_HANDLE;
}

void ImGUI::updatePipeline(VkRenderPass renderPass)
{
    if (renderPass == VK_NULL_HANDLE) {
        return;
    }

    if (pipeline_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(device_->getHandle(), pipeline_, nullptr);
        pipeline_ = VK_NULL_HANDLE;
    }

    // 顶点输入信息
    std::vector<VkVertexInputBindingDescription> viBinding = {
        {0, sizeof(ImDrawVert), VK_VERTEX_INPUT_RATE_VERTEX}
    };

    std::vector<VkVertexInputAttributeDescription> viAttrs = {
        vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, pos)),   // Loc 0: Pos
        vertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, uv)),    // Loc 1: UV
        vertexInputAttributeDescription(0, 2, VK_FORMAT_R8G8B8A8_UNORM, offsetof(ImDrawVert, col)),  // Loc 0: Color
    };

    pipeline_builder_.setVertexInputState(viBinding, viAttrs);

    std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates(1);
    blendAttachmentStates[0] = pipelineColorBlendAttachmentState(static_cast<VkColorComponentFlagBits>(0xf), VK_FALSE);
    blendAttachmentStates[0].alphaBlendOp = VK_BLEND_OP_ADD;
    blendAttachmentStates[0].colorBlendOp = VK_BLEND_OP_ADD;
    blendAttachmentStates[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blendAttachmentStates[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttachmentStates[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttachmentStates[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    pipeline_builder_.setColorBlendState(blendAttachmentStates);

    std::vector<VkDynamicState> dynamicStateEnables = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    pipeline_builder_.setDynamicState(dynamicStateEnables);

    pipeline_builder_.setDepthStencilState(VK_FALSE, VK_FALSE);

    pipeline_builder_.createPipeline(renderPass, pipeline_layout_, pipeline_);
}

void ImGUI::draw(VkCommandBuffer cmdBuffer)
{
    if (pipeline_ == VK_NULL_HANDLE) {
        return;
    }

    ImDrawData* imDrawData = ImGui::GetDrawData();

    // Create and grow vertex/index buffers if needed
    char* pVertices = nullptr;
    VkDescriptorBufferInfo VerticesView{};
    constant_buffer_->allocConstantBuffer(imDrawData->TotalVtxCount * sizeof(ImDrawVert),
                                          (void**) &pVertices,
                                          VerticesView);

    char* pIndices = nullptr;
    VkDescriptorBufferInfo IndicesView{};
    constant_buffer_->allocConstantBuffer(imDrawData->TotalIdxCount * sizeof(ImDrawIdx),
                                          (void**) &pIndices,
                                          IndicesView);

    auto* vtx_dst = (ImDrawVert*) pVertices;
    auto* idx_dst = (ImDrawIdx*) pIndices;
    for (int n = 0; n < imDrawData->CmdListsCount; n++) {
        const ImDrawList* cmd_list = imDrawData->CmdLists[n];
        std::memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
        std::memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
        vtx_dst += cmd_list->VtxBuffer.Size;
        idx_dst += cmd_list->IdxBuffer.Size;
    }

    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
    vkCmdBindDescriptorSets(cmdBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipeline_layout_,
                            0,
                            1,
                            &descriptor_set_[current_descriptor_index_],
                            0,
                            nullptr);
    current_descriptor_index_++;
    current_descriptor_index_ &= 127;

    ImGuiIO& io = ImGui::GetIO();
    push_const_block_.scale = glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
    push_const_block_.translate = glm::vec2(-1.0f);
    vkCmdPushConstants(cmdBuffer,
                       pipeline_layout_,
                       VK_SHADER_STAGE_VERTEX_BIT,
                       0,
                       sizeof(PushConstBlock),
                       &push_const_block_);
    int32_t vertexOffset = 0;
    int32_t indexOffset = 0;

    if (imDrawData->CmdListsCount > 0) {
        vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &VerticesView.buffer, &VerticesView.offset);
        vkCmdBindIndexBuffer(cmdBuffer, IndicesView.buffer, IndicesView.offset, VK_INDEX_TYPE_UINT16);

        for (int32_t i = 0; i < imDrawData->CmdListsCount; i++) {
            const ImDrawList* cmd_list = imDrawData->CmdLists[i];
            for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++) {
                const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[j];
                VkRect2D scissorRect;
                scissorRect.offset.x = std::max((int32_t) (pcmd->ClipRect.x), 0);
                scissorRect.offset.y = std::max((int32_t) (pcmd->ClipRect.y), 0);
                scissorRect.extent.width = (uint32_t) (pcmd->ClipRect.z - pcmd->ClipRect.x);
                scissorRect.extent.height = (uint32_t) (pcmd->ClipRect.w - pcmd->ClipRect.y);
                vkCmdSetScissor(cmdBuffer, 0, 1, &scissorRect);
                vkCmdDrawIndexed(cmdBuffer, pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);
                indexOffset += pcmd->ElemCount;
            }
            vertexOffset += cmd_list->VtxBuffer.Size;
        }
    }
}

} // yu::vk
