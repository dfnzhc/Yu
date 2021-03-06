//
// Created by 秋鱼 on 2022/6/3.
//

#pragma once

#include <vulkan/vulkan_core.h>

namespace yu::vk {

struct Properties
{
    ~Properties() = default;

    /**
     * @brief 查询系统可用的扩展与中间层
     */
    std::vector<VkLayerProperties> layer_properties{};
    std::vector<VkExtensionProperties> extension_properties{};

    /**
     * @brief 通过设置启用的扩展与中间层
     */
    std::vector<const char*> enabled_layers{};
    std::vector<const char*> enabled_extensions{};
    void* pNext = nullptr;

    bool addLayer(std::string_view layerName);
    bool addExtension(std::string_view extensionName);

    bool IsLayerExist(std::string_view layer);
    bool IsExtensionExist(std::string_view extension);

    bool IsLayerEnabled(std::string_view layer);
    bool IsExtensionEnabled(std::string_view extension);
};


} // namespace yu::vk