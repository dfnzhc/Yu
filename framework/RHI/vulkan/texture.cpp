//
// Created by 秋鱼 on 2022/6/19.
//

#include "texture.hpp"
#include "error.hpp"
#include "initializers.hpp"

namespace yu::vk {

void Texture::create(const VulkanDevice& device, VkImageCreateInfo& createInfo, std::string_view name)
{
    device_ = &device;

    bitmap_.mip_level = createInfo.mipLevels;
    info_.width = createInfo.extent.width;
    info_.height = createInfo.extent.height;
    info_.depth = createInfo.extent.depth;
    bitmap_.depth = createInfo.arrayLayers;
    format_ = createInfo.format;

#ifdef USE_VMA
    VmaAllocationCreateInfo imageAllocCreateInfo = {};
    imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    imageAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
    imageAllocCreateInfo.pUserData = const_cast<char*>(name.data());
    VmaAllocationInfo gpuImageAllocInfo = {};
    VK_CHECK(vmaCreateImage(device_->getAllocator(),
                            &createInfo,
                            &imageAllocCreateInfo,
                            &image_,
                            &image_allocation_,
                            &gpuImageAllocInfo));
#else
    /* Create image */
    VK_CHECK(vkCreateImage(device_->getHandle(), &createInfo, nullptr, &image_));

    VkMemoryRequirements mem_reqs;
    vkGetImageMemoryRequirements(device_->getHandle(), image_, &mem_reqs);

    auto alloc_info = memoryAllocateInfo();
    alloc_info.pNext = nullptr;
    alloc_info.allocationSize = 0;
    alloc_info.allocationSize = mem_reqs.size;
    alloc_info.memoryTypeIndex = 0;

    bool pass = device_->getProperties().getMemoryType(mem_reqs.memoryTypeBits,
                                                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                                       &alloc_info.memoryTypeIndex);
    assert(pass);

    /* Allocate memory */
    VK_CHECK(vkAllocateMemory(device_->getHandle(), &alloc_info, nullptr, &device_memory_));

    /* bind memory */
    VK_CHECK(vkBindImageMemory(device_->getHandle(), image_, device_memory_, 0));
#endif
}

void Texture::createRenderTarget(const VulkanDevice& device,
                                 uint32_t width,
                                 uint32_t height,
                                 VkFormat format,
                                 VkSampleCountFlagBits sampleBits,
                                 VkImageUsageFlagBits usageBits,
                                 VkImageCreateFlagBits createBits,
                                 std::string_view name)
{
    auto imageInfo = imageCreateInfo();
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = format;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = sampleBits;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.queueFamilyIndexCount = 0;
    imageInfo.pQueueFamilyIndices = nullptr;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.usage = usageBits;
    imageInfo.flags = createBits;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;

    create(device, imageInfo, name);
}

void Texture::createDepthStencil(const VulkanDevice& device,
                                 uint32_t width,
                                 uint32_t height,
                                 VkFormat format,
                                 VkSampleCountFlagBits sampleBits,
                                 std::string_view name)
{
    auto imageInfo = imageCreateInfo();
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = format;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = sampleBits;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.queueFamilyIndexCount = 0;
    imageInfo.pQueueFamilyIndices = nullptr;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.flags = 0;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;

    create(device, imageInfo, name);
}

void Texture::destory()
{
#ifdef USE_VMA
    if (image_ != VK_NULL_HANDLE) {
        vmaDestroyImage(device_->getAllocator(), image_, image_allocation_);
        image_ = VK_NULL_HANDLE;
    }
#else
    if (device_memory_ != VK_NULL_HANDLE) {
        vkDestroyImage(device_->getHandle(), image_, nullptr);
        vkFreeMemory(device_->getHandle(), device_memory_, nullptr);
        device_memory_ = VK_NULL_HANDLE;
    }
#endif
}

void Texture::createRTV(VkImageView* pImageView, int mipLevel, VkFormat format)
{
    auto viewInfo = imageViewCreateInfo();
    viewInfo.image = image_;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;

    if (bitmap_.depth > 1) {
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        viewInfo.subresourceRange.layerCount = bitmap_.depth;
    } else {
        viewInfo.subresourceRange.layerCount = 1;
    }
    if (format == VK_FORMAT_UNDEFINED)
        viewInfo.format = format_;
    else
        viewInfo.format = format;
    if (format_ == VK_FORMAT_D32_SFLOAT)
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    else
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    if (mipLevel == -1) {
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = bitmap_.mip_level;
    } else {
        viewInfo.subresourceRange.baseMipLevel = mipLevel;
        viewInfo.subresourceRange.levelCount = 1;
    }

    viewInfo.subresourceRange.baseArrayLayer = 0;
    VK_CHECK(vkCreateImageView(device_->getHandle(), &viewInfo, nullptr, pImageView));
}

void Texture::createSRV(VkImageView* pImageView, int mipLevel)
{
    auto viewInfo = imageViewCreateInfo();
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image_;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    if (bitmap_.depth > 1) {
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        viewInfo.subresourceRange.layerCount = bitmap_.depth;
    } else {
        viewInfo.subresourceRange.layerCount = 1;
    }
    viewInfo.format = format_;
    if (format_ == VK_FORMAT_D32_SFLOAT)
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    else
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    if (mipLevel == -1) {
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = bitmap_.mip_level;
    } else {
        viewInfo.subresourceRange.baseMipLevel = mipLevel;
        viewInfo.subresourceRange.levelCount = 1;
    }

    viewInfo.subresourceRange.baseArrayLayer = 0;

    VK_CHECK(vkCreateImageView(device_->getHandle(), &viewInfo, nullptr, pImageView));
}

void Texture::createDSV(VkImageView* pImageView)
{
    auto viewInfo = imageViewCreateInfo();
    viewInfo.image = image_;
    viewInfo.format = format_;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;

    if (format_ == VK_FORMAT_D16_UNORM_S8_UINT || format_ == VK_FORMAT_D24_UNORM_S8_UINT
        || format_ == VK_FORMAT_D32_SFLOAT_S8_UINT) {
        viewInfo.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }

    bitmap_.mip_level = 1;

    VK_CHECK(vkCreateImageView(device_->getHandle(), &viewInfo, nullptr, pImageView));
}

void Texture::createCubeSRV(VkImageView* pImageView)
{
    auto viewInfo = imageViewCreateInfo();
    viewInfo.image = image_;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    viewInfo.format = format_;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = bitmap_.mip_level;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = bitmap_.depth;

    VK_CHECK(vkCreateImageView(device_->getHandle(), &viewInfo, nullptr, pImageView));
}

void Texture::createFromFile2D(const VulkanDevice& device,
                               UploadHeap& uploadHeap,
                               std::string_view fileName,
                               VkImageUsageFlags flags)
{
    device_ = &device;

    bitmap_ = LoadTextureFormFile(fileName);

    if (bitmap_.bitmap_format == BitmapFormat::UnsignedByte) {
        format_ = VK_FORMAT_R8G8B8A8_UNORM;
    } else if (bitmap_.bitmap_format == BitmapFormat::Float) {
        format_ = VK_FORMAT_R32G32B32A32_SFLOAT;
    }

    createVulkanImage(bitmap_.name);
    upload(uploadHeap);
}

void Texture::createVulkanImage(std::string_view name, VkImageUsageFlags flags)
{
    auto imgInfo = imageCreateInfo();
    imgInfo.imageType = VK_IMAGE_TYPE_2D;
    imgInfo.format = format_;
    imgInfo.extent.width = bitmap_.width;
    imgInfo.extent.height = bitmap_.height;
    imgInfo.extent.depth = 1;
    imgInfo.mipLevels = bitmap_.mip_level;
    imgInfo.arrayLayers = bitmap_.depth;
    if (bitmap_.depth == 6)
        imgInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    imgInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imgInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | flags;
    imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

#ifdef USE_VMA
    VmaAllocationCreateInfo imageAllocCreateInfo = {};
    imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    imageAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
    imageAllocCreateInfo.pUserData = const_cast<char*>(name.data());
    VmaAllocationInfo gpuImageAllocInfo = {};
    VK_CHECK(vmaCreateImage(device_->getAllocator(),
                            &imgInfo,
                            &imageAllocCreateInfo,
                            &image_,
                            &image_allocation_,
                            &gpuImageAllocInfo));
#else
    VK_CHECK(vkCreateImage(device_->getHandle(), &imgInfo, nullptr, &image_));

    VkMemoryRequirements mem_reqs;
    vkGetImageMemoryRequirements(device_->getHandle(), image_, &mem_reqs);

    auto alloc_info = memoryAllocateInfo();
    alloc_info.allocationSize = mem_reqs.size;
    alloc_info.memoryTypeIndex = 0;

    bool pass = device_->getProperties().getMemoryType(mem_reqs.memoryTypeBits,
                                                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                                       &alloc_info.memoryTypeIndex);
    assert(pass && "No mappable, coherent memory");

    VK_CHECK(vkAllocateMemory(device_->getHandle(), &alloc_info, NULL, &device_memory_));

    VK_CHECK(vkBindImageMemory(device_->getHandle(), image_, device_memory_, 0));
#endif
}

void Texture::upload(UploadHeap& uploadHeap)
{
    // 前置 barrier
    {
        auto barrier = imageMemoryBarrier();
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.image = image_;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = bitmap_.mip_level;
        barrier.subresourceRange.layerCount = bitmap_.depth;
        uploadHeap.addImagePreBarrier(barrier);
    }

    // 上传图片
    auto bytesPerPixel = static_cast<uint32_t>(SizeOfFormat(format_));

    uint32_t width = bitmap_.width, height = bitmap_.height;
    uint32_t imgDataOffset = 0;
    for (auto face = 0; face < static_cast<int>(bitmap_.depth); ++face) {
        for (auto mip = 0; mip < static_cast<int>(bitmap_.mip_level); ++mip) {
            auto w = std::max<uint32_t>(width >> mip, 1);
            auto h = std::max<uint32_t>(height >> mip, 1);

            auto uploadSize = static_cast<uint32_t>(w * h * bytesPerPixel);
            auto* pixels = uploadHeap.beginAlloc(uploadSize, 512);

            // 如果没有成功从上传堆中分配到足够大小的空间，先把已经暂存的数据上传到设备，再进行尝试分配
            if (!pixels) {
                uploadHeap.flushAndFinish(true);
                pixels = uploadHeap.alloc(uploadSize, 512);

                assert(pixels);
            }

            // 拷贝图片数据到上传堆
            // ...
            const auto* img = bitmap_.pixels.data() + imgDataOffset;
            std::memcpy(pixels, img, uploadSize);
            imgDataOffset += static_cast<uint32_t>(w * h * bytesPerPixel);

            uploadHeap.endAlloc();

            VkBufferImageCopy region{};
            region.bufferOffset = static_cast<uint32_t>(pixels - uploadHeap.basePtr());;
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.layerCount = 1;
            region.imageSubresource.baseArrayLayer = face;
            region.imageSubresource.mipLevel = mip;
            region.imageExtent.width = w;
            region.imageExtent.height = h;
            region.imageExtent.depth = 1;
            uploadHeap.addImageCopy(image_, region);
        }
    }

    // 后置 barrier
    {
        auto barrier = imageMemoryBarrier();
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.image = image_;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.levelCount = bitmap_.mip_level;
        barrier.subresourceRange.layerCount = bitmap_.depth;
        uploadHeap.addImagePostBarrier(barrier);
    }
}

} // yu::vk