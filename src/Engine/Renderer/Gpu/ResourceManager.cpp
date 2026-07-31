#include "Engine/Renderer/Vulkan/Allocator.h"
#include "Engine/Renderer/Vulkan/Context.h"
#include "Engine/Core/Logger.h"

#include "ResourceManager.h"

namespace Lgt::Gpu {


void ResourceManager::Init() {
    LIGHTVK_INFO("ResourceManager Initialized");
}

void ResourceManager::Shutdown() {
    // Destroy all alive textures
    textures_.ForEachAlive([this](Texture& tex, TextureHandle handle) {
        if (tex.defaultView != VK_NULL_HANDLE) {
            vkDestroyImageView(Vulkan::g_Device->Logical(), tex.defaultView, nullptr);
        }
        if (tex.image != VK_NULL_HANDLE && !tex.isSwapchainImage) {
            Vulkan::g_Allocator->destroyImage(tex.image, tex.allocation);
        }
    });
    textures_.Clear();

    buffers_.ForEachAlive([this](Buffer& buf, BufferHandle handle) {
        if (buf.mapped)
            Vulkan::g_Allocator->unmap(buf.allocation);
        Vulkan::g_Allocator->destroyBuffer(buf.buffer, buf.allocation);
    });
    buffers_.Clear();

    LIGHTVK_INFO("ResourceManager Shutdown");
}

TextureHandle ResourceManager::CreateTexture(const TextureDesc& desc) {
    Texture tex{};
    tex.format           = desc.format;
    tex.width            = desc.width;
    tex.height           = desc.height;
    tex.mipLevels        = desc.mipLevels;
    tex.arrayLayers      = desc.arrayLayers;
    tex.debugName        = desc.debugName;
    tex.currentLayout    = VK_IMAGE_LAYOUT_UNDEFINED;
    tex.currentAccess    = 0;
    tex.currentStage     = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    tex.isSwapchainImage = false;

    VkImageCreateInfo imageInfo{};
    imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType     = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width  = desc.width;
    imageInfo.extent.height = desc.height;
    imageInfo.extent.depth  = desc.depth;
    imageInfo.mipLevels     = desc.mipLevels;
    imageInfo.arrayLayers   = desc.arrayLayers;
    imageInfo.format        = desc.format;
    imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage         = desc.usage;
    imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;

    if (!Vulkan::g_Allocator->createImage(imageInfo, desc.memoryUsage, 0, tex.image, tex.allocation)) {
        LIGHTVK_CRITICAL("Failed to create image in ResourceManager");
        return TextureHandle();
    }

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image                           = tex.image;
    viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format                          = desc.format;
    viewInfo.subresourceRange.aspectMask     = desc.aspectMask;
    viewInfo.subresourceRange.baseMipLevel   = 0;
    viewInfo.subresourceRange.levelCount     = desc.mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount     = desc.arrayLayers;

    if (vkCreateImageView(Vulkan::g_Device->Logical(), &viewInfo, nullptr, &tex.defaultView) != VK_SUCCESS) {
        LIGHTVK_CRITICAL("Failed to create image view in ResourceManager");
        Vulkan::g_Allocator->destroyImage(tex.image, tex.allocation);
        return TextureHandle();
    }

    return textures_.Allocate(tex);
}

void ResourceManager::DestroyTexture(TextureHandle handle) {
    if (!handle.IsValid())
        return;

    Texture* tex = textures_.Get(handle);
    if (!tex)
        return;

    if (tex->defaultView != VK_NULL_HANDLE) {
        vkDestroyImageView(Vulkan::g_Device->Logical(), tex->defaultView, nullptr);
    }
    if (tex->image != VK_NULL_HANDLE && !tex->isSwapchainImage) {
        Vulkan::g_Allocator->destroyImage(tex->image, tex->allocation);
    }

    textures_.Free(handle);
}

BufferHandle ResourceManager::CreateBuffer(const BufferDesc& desc) {
    VkBufferCreateInfo bufferci{};
    bufferci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferci.size  = desc.size;
    bufferci.usage = desc.usage;

    // Always require device address for our modern engine setup
    bufferci.usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    bufferci.sharingMode = Vulkan::g_Device->GraphicsFamily() == Vulkan::g_Device->TransferFamily() ? VK_SHARING_MODE_CONCURRENT
                                                                                                    : VK_SHARING_MODE_EXCLUSIVE;

    uint32_t queuefamilyindices[]  = {Vulkan::g_Device->GraphicsFamily(), Vulkan::g_Device->TransferFamily()};
    bufferci.queueFamilyIndexCount = 2;
    bufferci.pQueueFamilyIndices   = queuefamilyindices;

    Buffer buffer{};
    buffer.debugName = desc.debugName;
    buffer.size      = desc.size;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = desc.memoryUsage;
    if (desc.dynamic) {
        allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    }

    if (Vulkan::g_Allocator->createBuffer(bufferci, allocInfo, buffer.buffer, buffer.allocation)) {
        buffer.deviceAddress = getBufferDeviceAddress(buffer.buffer);
        if (desc.dynamic) {
            buffer.mapped = Vulkan::g_Allocator->map(buffer.allocation);
        }
        LIGHTVK_INFO("Created Buffer size: {}", desc.size);
        return buffers_.Allocate(buffer);
    }

    LIGHTVK_CRITICAL("Failed to create Buffer");
    return BufferHandle();
}

void ResourceManager::DestroyBuffer(BufferHandle handle) {
    if (!handle.IsValid())
        return;

    Buffer* buf = buffers_.Get(handle);
    if (!buf)
        return;

    if (buf->mapped)
        Vulkan::g_Allocator->unmap(buf->allocation);

    Vulkan::g_Allocator->destroyBuffer(buf->buffer, buf->allocation);
    buffers_.Free(handle);
}

} // namespace Lgt::Gpu
