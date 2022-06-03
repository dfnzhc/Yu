//
// Created by 秋鱼 on 2022/6/3.
//

#include "properties.hpp"
#include <logger.hpp>

namespace yu::vk {

bool Properties::addLayer(std::string_view layerName)
{
    if (IsLayerExist(layerName)) {
        enabled_layers.push_back(layerName.data());

        return true;
    }

    LOG_WARN("The layer {} has not been found", layerName);

    return false;
}

bool Properties::addExtension(std::string_view extensionName)
{
    if (IsExtensionExist(extensionName)) {
        enabled_extensions.push_back(extensionName.data());

        return true;
    }

    LOG_WARN("The extension {} has not been found", extensionName);

    return false;
}

bool Properties::IsLayerExist(std::string_view layer)
{
    return std::find_if(std::execution::par, layer_properties.begin(), layer_properties.end(),
                        [layer](const VkLayerProperties& layerProps) -> bool
                        {
                            return std::strcmp(layerProps.layerName, layer.data()) == 0;
                        }) != layer_properties.end();
}

bool Properties::IsExtensionExist(std::string_view extension)
{
    return std::find_if(std::execution::par, extension_properties.begin(), extension_properties.end(),
                        [extension](const VkExtensionProperties& extensionProps) -> bool
                        {
                            return std::strcmp(extensionProps.extensionName, extension.data()) == 0;
                        }) != extension_properties.end();
}

bool Properties::IsLayerEnabled(std::string_view layer)
{
    return std::find_if(std::execution::par, enabled_layers.begin(), enabled_layers.end(),
                        [layer](const char* layerName) -> bool
                        {
                            return std::strcmp(layerName, layer.data()) == 0;
                        }) != enabled_layers.end();
}

bool Properties::IsExtensionEnabled(std::string_view extension)
{
    return std::find_if(std::execution::par, enabled_extensions.begin(), enabled_extensions.end(),
                        [extension](const char* extensionName) -> bool
                        {
                            return std::strcmp(extensionName, extension.data()) == 0;
                        }) != enabled_extensions.end();
}

} // namespace yu::vk