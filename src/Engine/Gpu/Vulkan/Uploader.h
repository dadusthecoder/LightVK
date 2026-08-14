#pragma once
#include "Engine/Core/Core.h"

namespace Lgt::Vulkan {

struct TextureCopy {

    uint32_t srcMipLevel;
    uint32_t dstMipLevel;
    uint32_t srcArrayLayer;
    uint32_t dstArrayLayer;
    IVec3    srcOffset;
    IVec3    dstOffset;
    IVec3    extent;

    TextureCopy& setSrcMip(uint32_t mip) {
        srcMipLevel = mip;
        return *this;
    }
    TextureCopy& setSrcLayer(uint32_t layer) {
        srcArrayLayer = layer;
        return *this;
    }
    TextureCopy& setSrcOffset(IVec3 off) {
        srcOffset = off;
        return *this;
    }
    TextureCopy& setDstMip(uint32_t mip) {
        dstMipLevel = mip;
        return *this;
    }
    TextureCopy& setDstLayer(uint32_t layer) {
        dstArrayLayer = layer;
        return *this;
    }
    TextureCopy& setDstOffset(IVec3 off) {
        dstOffset = off;
        return *this;
    }
    TextureCopy& setExtent(IVec3 ext) {
        extent = ext;
        return *this;
    }
    TextureCopy& setExtent(uint32_t w, uint32_t h, uint32_t d = 1) {
        extent = IVec3(w, h, d);
        return *this;
    }

    // Copy a full 2D/3D texture at mip 0, layer 0
    static TextureCopy FullTexture(uint32_t w, uint32_t h, uint32_t d = 1) {
        return TextureCopy()
            .setExtent(w, h, d)
            .setSrcOffset(IVec3(0))
            .setDstOffset(IVec3(0))
            .setSrcMip(0)
            .setDstMip(0)
            .setSrcLayer(0)
            .setDstLayer(0);
    }

    // Copy a specific mip level in full
    static TextureCopy FullMip(uint32_t w, uint32_t h, uint32_t mip, uint32_t d = 1) {
        return TextureCopy().setSrcMip(mip).setDstMip(mip).setExtent(w >> mip ? w >> mip : 1, h >> mip ? h >> mip : 1, d);
    }

    // Copy one array layer to another (same or different texture)
    static TextureCopy Layer(uint32_t w, uint32_t h, uint32_t srcLayer, uint32_t dstLayer) {
        return TextureCopy().setSrcLayer(srcLayer).setDstLayer(dstLayer).setExtent(w, h);
    }

    // Copy a sub-region at the same location in both src and dst
    static TextureCopy Region(IVec3 offset, IVec3 ext) {
        return TextureCopy().setSrcOffset(offset).setDstOffset(offset).setExtent(ext);
    }

    // Copy a sub-region to a different location in dst
    static TextureCopy Blit(IVec3 srcOff, IVec3 dstOff, IVec3 ext) {
        return TextureCopy().setSrcOffset(srcOff).setDstOffset(dstOff).setExtent(ext);
    }
};
    
class VulkanLoadTimeStagingUploader {
public:
    explicit VulkanLoadTimeStagingUploader();
    ~VulkanLoadTimeStagingUploader();

    // Non-copyable, non-movable
    VulkanLoadTimeStagingUploader(const VulkanLoadTimeStagingUploader&)            = delete;
    VulkanLoadTimeStagingUploader& operator=(const VulkanLoadTimeStagingUploader&) = delete;
    VulkanLoadTimeStagingUploader(VulkanLoadTimeStagingUploader&&)                 = delete;
    VulkanLoadTimeStagingUploader& operator=(VulkanLoadTimeStagingUploader&&)      = delete;

    // Queue a buffer upload — data is copied to CPU-side staging immediately
    void UploadBuffer(VkBuffer dst, const void* data, uint32_t size, uint32_t dstOffset = 0);

    // Queue a texture upload — data is copied to CPU-side staging immediately
    void uploadTexture(VkImage dst, const void* data, uint32_t size, const TextureCopy& region);

    // Execute all queued uploads in a single GPU submission, then free staging memory.
    // Call once after all assets are queued. Safe to call on empty queue (no-op).
    void Flush();

    // Returns total bytes queued so far (before flush)
    uint32_t pendingBytes() const { return m_TotalSize; }

private:
    uint32_t pushData(const void* data, uint32_t size, uint32_t alignment = 256);

    struct BufferUpload {
        VkBuffer dst;
        uint32_t stagingOffset;
        uint32_t dstOffset;
        uint32_t size;
    };

    struct TextureUpload {
        VkImage     dst;
        uint32_t    stagingOffset;
        uint32_t    size;
        TextureCopy region;
    };

    VkCommandPool   m_Pool  = VK_NULL_HANDLE;
    VkCommandBuffer m_Cmd   = VK_NULL_HANDLE;
    VkFence         m_Fence = VK_NULL_HANDLE;

    std::vector<uint8_t>       m_CPUData; // all upload data packed linearly
    std::vector<BufferUpload>  m_BufferUploads;
    std::vector<TextureUpload> m_TextureUploads;
    uint32_t                   m_TotalSize = 0;
};
} // namespace Lgt::Vulkan