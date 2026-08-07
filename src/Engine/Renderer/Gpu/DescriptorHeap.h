#pragma once
#include "Engine/Renderer/Vulkan/Helpers.h"
#include "Engine/Renderer/Gpu/Resource.h"

namespace Lgt::Gpu {
class DescriptorHeap {
public:
    DescriptorHeap(size_t size, bool isResorceHeap = true, bool isSamplerHeap = false);
    ~DescriptorHeap();

    uint32_t AllocateSSBO(const BufferHandle& buffer);
    uint32_t AllocateUBO(const BufferHandle& buffer);
    uint32_t AllocateTexture(const TextureHandle& handle, const VkImageViewCreateInfo& view, VkImageLayout layout);
    uint32_t AllocateSampler(const VkSamplerCreateInfo& samplerInfo);

    size_t          GetSize() { return _total_size; }
    VkDeviceAddress GetBufferAddress() { return m_DeviceAddress; }
    size_t          GerReservedRangeOffset() { return _reserved_range_offset; }
    size_t          GetReservedRangeSize() { return _reserved_range_size; }

private:
    size_t _total_size  = 0;
    size_t _usable_size = 0;

    VkBuffer        m_Buffer        = VK_NULL_HANDLE;
    VkDeviceMemory  m_DeviceMemory  = VK_NULL_HANDLE;
    VkDeviceAddress m_DeviceAddress = 0;

    size_t _reserved_range_offset = 0;
    size_t _reserved_range_size   = 0;
    size_t _texture_offset        = 0;
    size_t _buffer_offset         = 0;
    size_t _sampler_offset        = 0;

    std::vector<size_t> m_FreeList;
};
} // namespace Lgt::Gpu
