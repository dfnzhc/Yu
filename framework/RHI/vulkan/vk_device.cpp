//
// Created by 秋鱼 on 2022/5/19.
//

#include "vk_device.hpp"
#include "vk_error.hpp"
#include "vk_initializers.hpp"
#include "vk_utils.hpp"

namespace ST::VK {

VulkanDevice::VulkanDevice(VkPhysicalDevice physicalDevice)
{
    assert(physicalDevice);
    this->physicalDevice = physicalDevice;

    // 存储物理设备的属性特征、限制、内存属性等，以便以后使用
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);
    vkGetPhysicalDeviceFeatures(physicalDevice, &features);
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

    // Queue family 的属性，用于在设备创建时设置需要的队列
    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
    assert(queueFamilyCount > 0);
    queueFamilyProperties.resize(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties.data());

    // 获取支持的扩展列表
    uint32_t extCount = 0;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, nullptr);
    if (extCount > 0) {
        std::vector<VkExtensionProperties> extensions(extCount);
        if (vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, &extensions.front())
            == VK_SUCCESS) {
            for (auto ext : extensions) {
                supportedExtensions.emplace_back(ext.extensionName);
            }
        }
    }
}

VulkanDevice::~VulkanDevice()
{
    if (commandPool) {
        vkDestroyCommandPool(logicalDevice, commandPool, nullptr);
    }
    if (logicalDevice) {
        vkDestroyDevice(logicalDevice, nullptr);
    }
}

/**
* 获得具有所有请求的内存类型的索引
*
* @param typeBits 所请求的资源所支持的每个内存类型的位掩码（来自VkMemoryRequirements）
* @param properties 要请求的内存类型的属性的位掩码
* @param (Optional) memTypeFound 指向一个bool的指针，如果找到了一个匹配的内存类型，该指针将被设置为true
* 
* @return 所请求的内存类型的索引
*
* @throw Throws 如果memTypeFound为空，并且没有找到支持所请求属性的内存类型，则抛出一个异常
*/
uint32_t VulkanDevice::getMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32* memTypeFound) const
{
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
        if ((typeBits & 1) == 1) {
            if ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                if (memTypeFound) {
                    *memTypeFound = true;
                }
                return i;
            }
        }
        typeBits >>= 1;
    }

    if (memTypeFound) {
        *memTypeFound = false;
        return 0;
    } else {
        throw std::runtime_error("Could not find a matching memory type");
    }
}

/**
* 获取支持所请求 queueFlags 的 queue family 的索引
*
* @param queueFlags 用于查找 queue family 索引的 Queue flags
*
* @return 标志所匹配的 queue family 索引的索引
*
* @throw Throws 如果找不到支持所要求标志的队列家族索引，则抛出一个异常
*/
uint32_t VulkanDevice::getQueueFamilyIndex(VkQueueFlagBits queueFlags) const
{
    // 专门用于计算的队列
    // 尝试找到一个支持计算但不支持图形的队列族索引
    if (queueFlags & VK_QUEUE_COMPUTE_BIT) {
        for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++) {
            if ((queueFamilyProperties[i].queueFlags & queueFlags)
                && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0)) {
                return i;
            }
        }
    }

    // 用于传输的专用队列
    // 尝试找到一个支持传输但不支持图形和计算的队列家族索引
    if (queueFlags & VK_QUEUE_TRANSFER_BIT) {
        for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++) {
            if ((queueFamilyProperties[i].queueFlags & queueFlags)
                && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0)
                && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0)) {
                return i;
            }
        }
    }

    // 对于其他队列类型，或者如果没有单独的计算队列存在，则返回第一个支持所要求的标志的队列
    for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++) {
        if (queueFamilyProperties[i].queueFlags & queueFlags) {
            return i;
        }
    }

    throw std::runtime_error("Could not find a matching queue family index");
}

/**
* 根据分配的物理设备创建逻辑设备，同时获得默认的 queue family 索引
*
* @param enabledFeatures 可以用来在设备创建时启用某些功能
* @param pNextChain 可选的指向扩展结构的指针链
* @param useSwapChain 对于 headless 渲染设置为false，以省略交换链设备扩展
* @param requestedQueueTypes 指定要从设备上请求的队列类型的位标志
*
* @return VkResult of the device creation call
*/
VkResult VulkanDevice::createLogicalDevice(VkPhysicalDeviceFeatures enabledFeatures,
                                           std::vector<const char*> enabledExtensions,
                                           void* pNextChain,
                                           bool useSwapChain,
                                           VkQueueFlags requestedQueueTypes)
{
    // 需要在创建逻辑设备时申请所需的队列
    // 由于Vulkan实现的 queue family 配置不同，这可能有点棘手，尤其是当应用程序要求不同的队列类型
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};

    // 获取请求的 queue family 类型的 queue family 索引
    // 注意，这些索引可能会重叠，这取决于实现方式
    const float defaultQueuePriority(0.0f);

    // 图形队列
    if (requestedQueueTypes & VK_QUEUE_GRAPHICS_BIT) {
        queueFamilyIndices.graphics = getQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT);
        VkDeviceQueueCreateInfo queueInfo{};
        queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.queueFamilyIndex = queueFamilyIndices.graphics;
        queueInfo.queueCount = 1;
        queueInfo.pQueuePriorities = &defaultQueuePriority;
        queueCreateInfos.push_back(queueInfo);
    } else {
        queueFamilyIndices.graphics = 0;
    }

    // 专门的计算队列
    if (requestedQueueTypes & VK_QUEUE_COMPUTE_BIT) {
        queueFamilyIndices.compute = getQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT);
        if (queueFamilyIndices.compute != queueFamilyIndices.graphics) {
            // 如果计算队列索引与图形队列不同，我们需要为计算队列提供额外的队列创建信息
            VkDeviceQueueCreateInfo queueInfo{};
            queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueInfo.queueFamilyIndex = queueFamilyIndices.compute;
            queueInfo.queueCount = 1;
            queueInfo.pQueuePriorities = &defaultQueuePriority;
            queueCreateInfos.push_back(queueInfo);
        }
    } else {
        // 否则，我们使用相同的队列
        queueFamilyIndices.compute = queueFamilyIndices.graphics;
    }

    // 专门的传输队列
    if (requestedQueueTypes & VK_QUEUE_TRANSFER_BIT) {
        queueFamilyIndices.transfer = getQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT);
        if ((queueFamilyIndices.transfer != queueFamilyIndices.graphics)
            && (queueFamilyIndices.transfer != queueFamilyIndices.compute)) {
            // 如果传输队列索引与图形队列索引不同，我们需要为传输队列创建一个额外的队列信息
            VkDeviceQueueCreateInfo queueInfo{};
            queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueInfo.queueFamilyIndex = queueFamilyIndices.transfer;
            queueInfo.queueCount = 1;
            queueInfo.pQueuePriorities = &defaultQueuePriority;
            queueCreateInfos.push_back(queueInfo);
        }
    } else {
        // 否则，我们使用相同的队列
        queueFamilyIndices.transfer = queueFamilyIndices.graphics;
    }

    // 创建逻辑设备
    std::vector<const char*> deviceExtensions(enabledExtensions);
    if (useSwapChain) {
        // 如果设备将用于通过交换链呈现给显示器，我们需要申请交换链扩展
        deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    }

    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());;
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.pEnabledFeatures = &enabledFeatures;

    // 如果有一个pNext(Chain)被传递，我们需要把它添加到设备创建信息中
    VkPhysicalDeviceFeatures2 physicalDeviceFeatures2{};
    if (pNextChain) {
        physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        physicalDeviceFeatures2.features = enabledFeatures;
        physicalDeviceFeatures2.pNext = pNextChain;
        deviceCreateInfo.pEnabledFeatures = nullptr;
        deviceCreateInfo.pNext = &physicalDeviceFeatures2;
    }

    // 如果存在调试标记扩展，则启用它（可能意味着存在调试工具）
    if (extensionSupported(VK_EXT_DEBUG_MARKER_EXTENSION_NAME)) {
        deviceExtensions.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
        enableDebugMarkers = true;
    }

    if (!deviceExtensions.empty()) {
        for (const char* enabledExtension : deviceExtensions) {
            if (!extensionSupported(enabledExtension)) {
                throw std::runtime_error(
                    "Enabled device extension " + std::string{enabledExtension} + " is not present at device level.");
            }
        }

        deviceCreateInfo.enabledExtensionCount = (uint32_t) deviceExtensions.size();
        deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
    }

    this->enabledFeatures = enabledFeatures;

    VkResult result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &logicalDevice);
    if (result != VK_SUCCESS) {
        return result;
    }

    // 为图形命令缓冲区创建一个默认的命令池
    commandPool = createCommandPool(queueFamilyIndices.graphics);

    return result;
}

/**
* 在设备上创建一个缓冲区
*
* @param usageFlags 缓冲区的使用标志位掩码(i.e. index, vertex, uniform buffer)
* @param memoryPropertyFlags 这个缓冲区的内存属性 (i.e. device local, host visible, coherent)
* @param size 缓冲区的大小，单位是byes
* @param buffer 指向获取的缓冲区句柄的指针
* @param memory 指向获取的内存句柄的指针
* @param data 指向创建后应该被复制到缓冲区的数据（可选，如果不设置，没有数据被复制过来）。
*
* @return VK_SUCCESS 如果缓冲区句柄和内存已经被创建，并且（可选择传递）数据已经被复制，则返回VK_SUCCESS
*/
VkResult VulkanDevice::createBuffer(VkBufferUsageFlags usageFlags,
                                    VkMemoryPropertyFlags memoryPropertyFlags,
                                    VkDeviceSize size,
                                    VkBuffer* buffer,
                                    VkDeviceMemory* memory,
                                    void* data)
{
    // 创建缓冲区句柄
    VkBufferCreateInfo bufCreateInfo = bufferCreateInfo(usageFlags, size);
    bufCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VK_CHECK(vkCreateBuffer(logicalDevice, &bufCreateInfo, nullptr, buffer));

    // 创建支持缓冲区句柄的内存
    VkMemoryRequirements memReqs;
    VkMemoryAllocateInfo memAlloc = memoryAllocateInfo();
    vkGetBufferMemoryRequirements(logicalDevice, *buffer, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    // 找到一个符合缓冲区属性的内存类型索引
    memAlloc.memoryTypeIndex = getMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags);
    // 如果缓冲区设置了VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT，也需要在分配时启用相应的标志
    VkMemoryAllocateFlagsInfoKHR allocFlagsInfo{};
    if (usageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
        allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
        allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
        memAlloc.pNext = &allocFlagsInfo;
    }
    VK_CHECK(vkAllocateMemory(logicalDevice, &memAlloc, nullptr, memory));

    // 如果传递了一个指向缓冲区数据的指针，则映射缓冲区并复制数据
    if (data != nullptr) {
        void* mapped;
        VK_CHECK(vkMapMemory(logicalDevice, *memory, 0, size, 0, &mapped));
        memcpy(mapped, data, size);
        // 如果没有要求主机一致性，则进行手动刷新，使写入的内容可见
        if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0) {
            VkMappedMemoryRange mappedRange = mappedMemoryRange();
            mappedRange.memory = *memory;
            mappedRange.offset = 0;
            mappedRange.size = size;
            vkFlushMappedMemoryRanges(logicalDevice, 1, &mappedRange);
        }
        vkUnmapMemory(logicalDevice, *memory);
    }

    // 将内存附加到缓冲区对象上
    VK_CHECK(vkBindBufferMemory(logicalDevice, *buffer, *memory, 0));

    return VK_SUCCESS;
}

/**
* 在设备上创建一个缓冲区
*
* @param usageFlags 缓冲区的使用标志位掩码 (i.e. index, vertex, uniform buffer)
* @param memoryPropertyFlags 缓冲区的内存属性 (i.e. device local, host visible, coherent)
* @param buffer 指向 vk::Vulkan 缓冲区对象的指针
* @param size 缓冲区的大小，以字节为单位
* @param data 指向创建后应复制到缓冲区的数据的指针（可选，如果不设置，没有数据被复制过来）
*
* @return VK_SUCCESS if buffer handle and memory have been created and (optionally passed) data has been copied
*/
VkResult VulkanDevice::createBuffer(VkBufferUsageFlags usageFlags,
                                    VkMemoryPropertyFlags memoryPropertyFlags,
                                    Buffer* buffer,
                                    VkDeviceSize size,
                                    void* data)
{
    buffer->device = logicalDevice;

    // 创建缓冲区句柄
    VkBufferCreateInfo bufCreateInfo = bufferCreateInfo(usageFlags, size);
    VK_CHECK(vkCreateBuffer(logicalDevice, &bufCreateInfo, nullptr, &buffer->buffer));

    // 创建支持缓冲区句柄的内存
    VkMemoryRequirements memReqs;
    VkMemoryAllocateInfo memAlloc = memoryAllocateInfo();
    vkGetBufferMemoryRequirements(logicalDevice, buffer->buffer, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    // 找到一个符合缓冲区属性的内存类型索引
    memAlloc.memoryTypeIndex = getMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags);
    // 如果缓冲区设置了VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT，也需要在分配时启用相应的标志
    VkMemoryAllocateFlagsInfoKHR allocFlagsInfo{};
    if (usageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
        allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
        allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
        memAlloc.pNext = &allocFlagsInfo;
    }
    VK_CHECK(vkAllocateMemory(logicalDevice, &memAlloc, nullptr, &buffer->memory));

    buffer->alignment = memReqs.alignment;
    buffer->size = size;
    buffer->usageFlags = usageFlags;
    buffer->memoryPropertyFlags = memoryPropertyFlags;

    // 如果传递了一个指向缓冲区数据的指针，则映射缓冲区并复制数据
    if (data != nullptr) {
        VK_CHECK(buffer->map());
        memcpy(buffer->mapped, data, size);
        if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
            buffer->flush();

        buffer->unmap();
    }

    // 初始化一个覆盖整个缓冲区大小的默认描述符
    buffer->setupDescriptor();

    // 将内存附加到缓冲区对象上
    return buffer->bind();
}

/**
* 使用 VkCmdCopyBuffer 将缓冲区数据从src复制到dst
* 
* @param src 指向要复制的源缓冲区的指针
* @param dst 指向要复制到的目标缓冲区的指针
* @param queue Pointer
* @param copyRegion (Optional) 指向复制区域的指针，如果是NULL，整个缓冲区都会被复制
*
* @note 源和目的指针必须设置适当的传输使用标志 (TRANSFER_SRC / TRANSFER_DST)
*/
void VulkanDevice::copyBuffer(Buffer* src, Buffer* dst, VkQueue queue, VkBufferCopy* copyRegion)
{
    assert(dst->size <= src->size);
    assert(src->buffer);
    VkCommandBuffer copyCmd = createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
    VkBufferCopy bufferCopy{};
    if (copyRegion == nullptr) {
        bufferCopy.size = src->size;
    } else {
        bufferCopy = *copyRegion;
    }

    vkCmdCopyBuffer(copyCmd, src->buffer, dst->buffer, 1, &bufferCopy);

    flushCommandBuffer(copyCmd, queue);
}

/** 
* 创建一个用于分配命令缓冲区的命令池
* 
* @param queueFamilyIndex 要创建命令池的队列的队列集合索引
* @param createFlags (Optional) 命令池创建标志 (Defaults to VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)
*
* @note  从创建的池中分配的命令缓冲区只能提交给具有相同集合索引的队列
*
* @return 创建的命令缓冲区的句柄
*/
VkCommandPool VulkanDevice::createCommandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags createFlags)
{
    VkCommandPoolCreateInfo cmdPoolInfo = {};
    cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolInfo.queueFamilyIndex = queueFamilyIndex;
    cmdPoolInfo.flags = createFlags;
    VkCommandPool cmdPool;
    VK_CHECK(vkCreateCommandPool(logicalDevice, &cmdPoolInfo, nullptr, &cmdPool));
    return cmdPool;
}

/**
* 从命令池中分配一个命令缓冲区
*
* @param level 新命令缓冲区的级别（主要或次要）
* @param pool 命令池，命令缓冲区将从该池中分配
* @param (Optional) 如果为真，将开始对新的命令缓冲区进行记录（vkBeginCommandBuffer） (Defaults to false)
*
* @return 分配的命令缓冲区的句柄
*/
VkCommandBuffer VulkanDevice::createCommandBuffer(VkCommandBufferLevel level, VkCommandPool pool, bool begin)
{
    VkCommandBufferAllocateInfo cmdBufAllocateInfo = commandBufferAllocateInfo(pool, level, 1);
    VkCommandBuffer cmdBuffer;
    VK_CHECK(vkAllocateCommandBuffers(logicalDevice, &cmdBufAllocateInfo, &cmdBuffer));
    // 如果有要求，开始向新的命令缓冲区进行记录
    if (begin) {
        VkCommandBufferBeginInfo cmdBufInfo = commandBufferBeginInfo();
        VK_CHECK(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));
    }
    return cmdBuffer;
}

VkCommandBuffer VulkanDevice::createCommandBuffer(VkCommandBufferLevel level, bool begin)
{
    return createCommandBuffer(level, commandPool, begin);
}

/**
* 完成命令缓冲区的记录，并将其提交给队列
*
* @param commandBuffer 要刷新的命令缓冲区
* @param queue 要提交命令缓冲区的队列
* @param pool 创建命令缓冲区的命令池
* @param free (Optional) 一旦命令缓冲区被提交就释放它 (默认为 true)
*
* @note 提交命令缓冲区的队列必须与它所分配的池子来自同一个队列集合索引
* @note 使用 fence 来确保命令缓冲区已经完成执行
*/
void VulkanDevice::flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, VkCommandPool pool, bool free)
{
    if (commandBuffer == VK_NULL_HANDLE) {
        return;
    }

    VK_CHECK(vkEndCommandBuffer(commandBuffer));

    VkSubmitInfo submitInfo = ST::VK::submitInfo();
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    // 创建 fence 以确保命令缓冲区已经执行完毕
    VkFenceCreateInfo fenceInfo = fenceCreateInfo(VK_FLAGS_NONE);
    VkFence fence;
    VK_CHECK(vkCreateFence(logicalDevice, &fenceInfo, nullptr, &fence));
    // 提交到队列中
    VK_CHECK(vkQueueSubmit(queue, 1, &submitInfo, fence));
    // 等待 fence 发出信号，表明命令缓冲区已经执行完毕
    VK_CHECK(vkWaitForFences(logicalDevice, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));
    vkDestroyFence(logicalDevice, fence, nullptr);
    if (free) {
        vkFreeCommandBuffers(logicalDevice, pool, 1, &commandBuffer);
    }
}

void VulkanDevice::flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free)
{
    return flushCommandBuffer(commandBuffer, queue, commandPool, free);
}

/**
* 检查一个扩展是否被（物理设备）所支持
*
* @param extension 要检查的扩展名称
*
* @return 如果该扩展被支持（存在于设备创建时读取的列表中）返回真
*/
bool VulkanDevice::extensionSupported(std::string extension)
{
    return (std::find(supportedExtensions.begin(), supportedExtensions.end(), extension) != supportedExtensions.end());
}

/**
* 从可能的深度（和模版）格式列表中选择最适合该设备的深度格式
*
* @param checkSamplingSupport 检查该格式是否可以被采样（例如用于着色器读取）
*
* @return 最适合当前设备的深度格式
*
* @throw 如果没有深度格式符合要求，会抛出一个异常
*/
VkFormat VulkanDevice::getSupportedDepthFormat(bool checkSamplingSupport)
{
    // 所有的深度格式都可能是可选的，所以我们需要找到一个合适的深度格式来使用
    std::vector<VkFormat> depthFormats = {
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM
    };

    for (auto& format : depthFormats) {
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);
        // 格式必须支持深度模板附着，以获得最佳的平铺效果
        if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            if (checkSamplingSupport) {
                if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)) {
                    continue;
                }
            }
            return format;
        }
    }
    throw std::runtime_error("Could not find a matching depth format");
}
} // namespace ST::VK
