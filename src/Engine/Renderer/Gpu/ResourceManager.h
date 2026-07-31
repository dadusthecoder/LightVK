#pragma once

#include "Engine/Renderer/Vulkan/Helpers.h"
#include "Engine/Renderer/Gpu/Resource.h"

namespace Lgt::Gpu {

struct TextureDesc {
    VkFormat           format      = VK_FORMAT_UNDEFINED;
    uint32_t           width       = 0;
    uint32_t           height      = 0;
    uint32_t           depth       = 1;
    uint32_t           mipLevels   = 1;
    uint32_t           arrayLayers = 1;
    VkImageUsageFlags  usage       = 0;
    VmaMemoryUsage     memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
    VkImageAspectFlags aspectMask  = VK_IMAGE_ASPECT_COLOR_BIT;
    std::string        debugName;

    static TextureDesc ShadowMap(uint32_t width, uint32_t height, std::string debugName = "ShadowMap") {
        TextureDesc desc;
        desc.width      = width;
        desc.height     = height;
        desc.format     = VK_FORMAT_D32_SFLOAT;
        desc.usage      = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        desc.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        desc.debugName  = std::move(debugName);
        return desc;
    }

    static TextureDesc
    Texture2D(uint32_t width, uint32_t height, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM, std::string debugName = "Texture2D") {
        TextureDesc desc;
        desc.width     = width;
        desc.height    = height;
        desc.format    = format;
        desc.usage     = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        desc.debugName = std::move(debugName);
        return desc;
    }

    static TextureDesc ColorAttachment(uint32_t    width,
                                       uint32_t    height,
                                       VkFormat    format    = VK_FORMAT_R8G8B8A8_UNORM,
                                       std::string debugName = "ColorAttachment") {
        TextureDesc desc;
        desc.width     = width;
        desc.height    = height;
        desc.format    = format;
        desc.usage     = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        desc.debugName = std::move(debugName);
        return desc;
    }

    static TextureDesc DepthAttachment(uint32_t    width,
                                       uint32_t    height,
                                       VkFormat    format    = VK_FORMAT_D32_SFLOAT,
                                       std::string debugName = "DepthAttachment") {
        TextureDesc desc;
        desc.width      = width;
        desc.height     = height;
        desc.format     = format;
        desc.usage      = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        desc.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        desc.debugName  = std::move(debugName);
        return desc;
    }

    static TextureDesc StorageImage(uint32_t width, uint32_t height, VkFormat format, std::string debugName = "StorageImage") {
        TextureDesc desc;
        desc.width     = width;
        desc.height    = height;
        desc.format    = format;
        desc.usage     = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        desc.debugName = std::move(debugName);
        return desc;
    }

    static TextureDesc TextureCube(uint32_t    width,
                                   uint32_t    height,
                                   VkFormat    format    = VK_FORMAT_R8G8B8A8_UNORM,
                                   std::string debugName = "TextureCube") {
        TextureDesc desc;
        desc.width       = width;
        desc.height      = height;
        desc.arrayLayers = 6;
        desc.format      = format;
        desc.usage       = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        desc.debugName   = std::move(debugName);
        return desc;
    }
};

struct BufferDesc {
    size_t             size        = 0;
    VkBufferUsageFlags usage       = 0;
    VmaMemoryUsage     memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
    bool               dynamic     = false;
    std::string        debugName;

    static BufferDesc SSBO(size_t size, bool dynamic = false, std::string debugName = "SSBO") {
        BufferDesc desc;
        desc.size        = size;
        desc.dynamic     = dynamic;
        desc.memoryUsage = dynamic ? VMA_MEMORY_USAGE_CPU_TO_GPU : VMA_MEMORY_USAGE_GPU_ONLY;
        desc.usage       = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        if (!dynamic)
            desc.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        desc.debugName = std::move(debugName);
        return desc;
    }

    static BufferDesc UBO(size_t size, std::string debugName = "UBO") {
        BufferDesc desc;
        desc.size        = size;
        desc.dynamic     = true;
        desc.memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        desc.usage       = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        desc.debugName   = std::move(debugName);
        return desc;
    }

    static BufferDesc VBO(size_t size, std::string debugName = "VBO") {
        BufferDesc desc;
        desc.size        = size;
        desc.dynamic     = false;
        desc.memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
        desc.usage       = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        desc.debugName   = std::move(debugName);
        return desc;
    }

    static BufferDesc IBO(size_t size, std::string debugName = "IBO") {
        BufferDesc desc;
        desc.size        = size;
        desc.dynamic     = false;
        desc.memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
        desc.usage       = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        desc.debugName   = std::move(debugName);
        return desc;
    }

    static BufferDesc Staging(size_t size, std::string debugName = "Staging") {
        BufferDesc desc;
        desc.size        = size;
        desc.dynamic     = true;
        desc.memoryUsage = VMA_MEMORY_USAGE_CPU_ONLY;
        desc.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        desc.debugName   = std::move(debugName);
        return desc;
    }
};

class ResourceManager {
public:
    void Init();
    void Shutdown();

    TextureHandle CreateTexture(const TextureDesc& desc);
    void          DestroyTexture(TextureHandle handle);

    Texture* GetTexture(TextureHandle handle) { return textures_.Get(handle); }
    Buffer*  GetBuffer(BufferHandle handle) { return buffers_.Get(handle); }

    BufferHandle CreateBuffer(const BufferDesc& desc);
    void         DestroyBuffer(BufferHandle handle);

private:
    ResourcePool<Texture, TextureHandle> textures_;
    ResourcePool<Buffer, BufferHandle>   buffers_;
};


} // namespace Lgt::Gpu
