//
// Created by 秋鱼 on 2022/7/8.
//

#include <logger.hpp>
#include "model_obj.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <common/common.hpp>

namespace std {
template<>
struct ::std::hash<yu::vk::VertexObj>
{
    size_t operator()(yu::vk::VertexObj const& vertex) const
    {
        return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.normal) << 1)) >> 1)
            ^ (hash<glm::vec2>()(vertex.uv) << 1);
    }
};
}

namespace yu::vk {

void ModelObj::load(std::string_view fileName, std::string_view basePath, bool triangulate)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn;
    std::string err;
    auto fullPath = yu::GetModelFile(std::string{basePath.data()} + fileName.data());
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
                                fullPath.c_str(),
                                yu::GetModelFile(basePath.data()).c_str(), triangulate);

    if (!err.empty()) {
        LOG_ERROR("{}", err);
    }

    if (!ret) {
        LOG_FATAL("Failed to load/parse .obj file: {}.", fileName);
    }

    std::unordered_map<VertexObj, uint32_t> uniqueVertices{};
    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            VertexObj vertex{};

            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            vertex.normal = {
                attrib.normals[3 * index.normal_index + 0],
                attrib.normals[3 * index.normal_index + 1],
                attrib.normals[3 * index.normal_index + 2]
            };

            if (!attrib.texcoords.empty()) {
                vertex.uv = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                };
            }

            auto mat = std::find_if(materials.begin(), materials.end(), [&shape](const tinyobj::material_t& mat)
            {
                return mat.name == shape.name;
            });
            
            vertex.color = {
                mat->diffuse[0], mat->diffuse[1], mat->diffuse[2]
            };

            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<uint32_t>(obj_.vertices.size());
                obj_.vertices.push_back(vertex);
            }

            obj_.indices.push_back(uniqueVertices[vertex]);
        }
    }
}

void ModelObj::setPipelineVertexInput(std::vector<VkVertexInputBindingDescription>& bindingDesc,
                                      std::vector<VkVertexInputAttributeDescription>& attrDesc)
{
    bindingDesc = {VertexObj::getBindingDescription()};
    attrDesc = VertexObj::getAttributeDescriptions();
}

void ModelObj::allocMemory(StaticBuffer& staticBuffer)
{
    staticBuffer.allocBuffer(static_cast<uint32_t>(obj_.vertices.size()),
                             sizeof(VertexObj),
                             obj_.vertices.data(),
                             &vertex_info_);

    staticBuffer.allocBuffer(static_cast<uint32_t>(obj_.indices.size()),
                             sizeof(uint32_t),
                             obj_.indices.data(),
                             &index_info_);
}

void ModelObj::draw(VulkanPipeline& pipeline, VkCommandBuffer cmdBuffer, VkDescriptorBufferInfo* pConstantBuffer, VkDescriptorSet descriptorSet)
{
    pipeline.drawIndexed(cmdBuffer, static_cast<uint32_t>(obj_.indices.size()),
                         &vertex_info_, &index_info_, pConstantBuffer, descriptorSet);
}

} // yu::vk