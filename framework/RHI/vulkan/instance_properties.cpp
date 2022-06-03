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


} // yu::vk