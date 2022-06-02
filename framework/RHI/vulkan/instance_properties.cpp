//
// Created by 秋鱼 on 2022/6/1.
//

#include <logger.hpp>
#include "instance_properties.hpp"
#include "error.hpp"

namespace yu::vk {

InstanceProperties::InstanceProperties()
{
    // 查询可用的实例层
    uint32_t layerPropertyCount = 0;
    VK_CHECK(vkEnumerateInstanceLayerProperties(&layerPropertyCount, nullptr));
    if (layerPropertyCount > 0) {
        layer_properties.resize(layerPropertyCount);
        VK_CHECK(vkEnumerateInstanceLayerProperties(&layerPropertyCount, layer_properties.data()));
    }

    // 查询可用的扩展
    uint32_t extensionPropertyCount = 0;
    VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &extensionPropertyCount, nullptr));
    if (extensionPropertyCount > 0) {
        extension_properties.resize(extensionPropertyCount);
        VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &extensionPropertyCount, extension_properties.data()));
    }
}

bool InstanceProperties::addLayer(std::string_view layerName)
{
    if (IsLayerExist(layerName)) {
        enabled_layers.push_back(layerName.data());

        return true;
    }

    LOG_WARN("The instance layer {} has not been found", layerName);

    return false;
}

bool InstanceProperties::addExtension(std::string_view extensionName)
{
    if (IsExtensionExist(extensionName)) {
        enabled_extensions.push_back(extensionName.data());

        return true;
    }

    LOG_WARN("The instance extension {} has not been found", extensionName);

    return false;
}

bool InstanceProperties::IsLayerExist(std::string_view layer)
{
    return std::find_if(std::execution::par, layer_properties.begin(), layer_properties.end(),
                        [layer](const VkLayerProperties& layerProps) -> bool
                        {
                            return std::strcmp(layerProps.layerName, layer.data());
                        }) != layer_properties.end();
}

bool InstanceProperties::IsExtensionExist(std::string_view extension)
{
    return std::find_if(std::execution::par, extension_properties.begin(), extension_properties.end(),
                        [extension](const VkExtensionProperties& extensionProps) -> bool
                        {
                            return std::strcmp(extensionProps.extensionName, extension.data());
                        }) != extension_properties.end();
}

bool InstanceProperties::IsLayerEnabled(std::string_view layer)
{
    return std::find_if(std::execution::par, enabled_layers.begin(), enabled_layers.end(),
                        [layer](const char* layerName) -> bool
                        {
                            return std::strcmp(layerName, layer.data());
                        }) != enabled_layers.end();
}

bool InstanceProperties::IsExtensionEnabled(std::string_view extension)
{
    return std::find_if(std::execution::par, enabled_extensions.begin(), enabled_extensions.end(),
                        [extension](const char* extensionName) -> bool
                        {
                            return std::strcmp(extensionName, extension.data());
                        }) != enabled_extensions.end();
}

} // yu::vk