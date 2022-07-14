//
// Created by 秋鱼 on 2022/7/8.
//

#pragma once

#include "static_buffer.hpp"
#include "pipeline.hpp"

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/hash.hpp>

namespace yu::vk {

// 简单实现的 obj 模型类，用于加载一些只有顶点颜色的模型，纹理需要额外设置
struct VertexObj
{
    glm::vec3 pos{};
    glm::vec3 normal{};
    glm::vec2 uv{};

    glm::vec3 color{};

    static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(VertexObj);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions()
    {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions(4);

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(VertexObj, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(VertexObj, normal);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(VertexObj, uv);

        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[3].offset = offsetof(VertexObj, color);

        return attributeDescriptions;
    }

    bool operator==(const VertexObj& other) const
    {
        return pos == other.pos && normal == other.normal && uv == other.uv && color == other.color;
    }
};

struct ModelDataObj
{
    std::vector<VertexObj> vertices;
    std::vector<uint32_t> indices;
};

class ModelObj
{
public:
    ModelObj() = default;
    ~ModelObj() = default;

    void load(std::string_view fileName, std::string_view basePath = "", bool triangulate = true);

    static void SetPipelineVertexInput(std::vector<VkVertexInputBindingDescription>& bindingDesc,
                                       std::vector<VkVertexInputAttributeDescription>& attrDesc);

    void allocMemory(StaticBuffer& staticBuffer);

    void draw(VulkanPipeline& pipeline,
              VkCommandBuffer cmdBuffer,
              VkDescriptorBufferInfo* pConstantBuffer,
              VkDescriptorSet descriptorSet);

private:
    ModelDataObj obj_;
    VkDescriptorBufferInfo vertex_info_{};
    VkDescriptorBufferInfo index_info_{};
};

} // yu::vk