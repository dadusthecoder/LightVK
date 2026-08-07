#pragma once

#include "Engine/Renderer/Vulkan/Helpers.h"
#include "Engine/Renderer/Gpu/Resource.h"

namespace Lgt::Gpu {

struct TextureDesc {
    VkFormat           format = VK_FORMAT_UNDEFINED;
    Extent             extent;
    uint32_t           depth       = 1;
    uint32_t           mipLevels   = 1;
    uint32_t           arrayLayers = 1;
    VkImageUsageFlags  usage       = 0;
    VmaMemoryUsage     memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
    VkImageAspectFlags aspectMask  = VK_IMAGE_ASPECT_COLOR_BIT;
    std::string        debugName;

    static TextureDesc ShadowMap(Extent extent, std::string debugName = "ShadowMap") {
        TextureDesc desc;
        desc.extent     = extent;
        desc.format     = VK_FORMAT_D32_SFLOAT;
        desc.usage      = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        desc.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        desc.debugName  = std::move(debugName);
        return desc;
    }

    static TextureDesc Texture2D(Extent extent, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM, std::string debugName = "Texture2D") {
        TextureDesc desc;
        desc.extent    = extent;
        desc.format    = format;
        desc.usage     = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        desc.debugName = std::move(debugName);
        return desc;
    }

    static TextureDesc
    ColorAttachment(Extent extent, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM, std::string debugName = "ColorAttachment") {
        TextureDesc desc;
        desc.extent    = extent;
        desc.format    = format;
        desc.usage     = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        desc.debugName = std::move(debugName);
        return desc;
    }

    static TextureDesc
    DepthAttachment(Extent extent, VkFormat format = VK_FORMAT_D32_SFLOAT, std::string debugName = "DepthAttachment") {
        TextureDesc desc;
        desc.extent     = extent;
        desc.format     = format;
        desc.usage      = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        desc.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        desc.debugName  = std::move(debugName);
        return desc;
    }

    static TextureDesc StorageImage(Extent extent, VkFormat format, std::string debugName = "StorageImage") {
        TextureDesc desc;
        desc.extent    = extent;
        desc.format    = format;
        desc.usage     = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        desc.debugName = std::move(debugName);
        return desc;
    }

    static TextureDesc
    TextureCube(Extent extent, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM, std::string debugName = "TextureCube") {
        TextureDesc desc;
        desc.extent      = extent;
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

struct SamplerDesc {
    VkFilter              magFilter               = VK_FILTER_LINEAR;
    VkFilter              minFilter               = VK_FILTER_LINEAR;
    VkSamplerMipmapMode   mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    VkSamplerAddressMode  addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    VkSamplerAddressMode  addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    VkSamplerAddressMode  addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    float                 mipLodBias              = 0.0f;
    VkBool32              anisotropyEnable        = VK_FALSE;
    float                 maxAnisotropy           = 1.0f;
    VkBool32              compareEnable           = VK_FALSE;
    VkCompareOp           compareOp               = VK_COMPARE_OP_ALWAYS;
    float                 minLod                  = 0.0f;
    float                 maxLod                  = VK_LOD_CLAMP_NONE;
    VkBorderColor         borderColor             = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    VkBool32              unnormalizedCoordinates = VK_FALSE;
    std::string           debugName;

    VkSamplerCreateInfo ToVkCreateInfo() const {
        VkSamplerCreateInfo info{};
        info.sType                  = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        info.magFilter              = magFilter;
        info.minFilter              = minFilter;
        info.mipmapMode             = mipmapMode;
        info.addressModeU           = addressModeU;
        info.addressModeV           = addressModeV;
        info.addressModeW           = addressModeW;
        info.mipLodBias              = mipLodBias;
        info.anisotropyEnable        = anisotropyEnable;
        info.maxAnisotropy           = maxAnisotropy;
        info.compareEnable           = compareEnable;
        info.compareOp               = compareOp;
        info.minLod                  = minLod;
        info.maxLod                  = maxLod;
        info.borderColor             = borderColor;
        info.unnormalizedCoordinates = unnormalizedCoordinates;
        return info;
    }
};

class ResourceManager {
public:
    void Init();
    void Shutdown();

    TextureHandle CreateTexture(const TextureDesc& desc);
    void          DestroyTexture(TextureHandle handle);
    SamplerHandle CreateSampler(const SamplerDesc& desc);
    void          DestroySampler(SamplerHandle handle);

    Texture* GetTexture(TextureHandle handle) { return _textures.Get(handle); }
    Sampler* GetSampler(SamplerHandle handle) { return _samplers.Get(handle); }
    Buffer*  GetBuffer(BufferHandle handle) { return _buffers.Get(handle); }

    BufferHandle CreateBuffer(const BufferDesc& desc);
    void         DestroyBuffer(BufferHandle handle);

private:
    ResourcePool<Texture, TextureHandle> _textures;
    ResourcePool<Sampler, SamplerHandle> _samplers;
    ResourcePool<Buffer, BufferHandle>   _buffers;
};

} // namespace Lgt::Gpu
