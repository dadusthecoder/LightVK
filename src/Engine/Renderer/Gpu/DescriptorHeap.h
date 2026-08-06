#pragma once
#include "Engine/Renderer/Vulkan/Helpers.h"
#include "Engine/Renderer/Gpu/Resource.h"

namespace Lgt::Gpu {
class DescriptorHeap {
public:
    DescriptorHeap(size_t size, bool isResorceHeap = true, bool isSamplerHeap = false);
    ~DescriptorHeap();

    VkDeviceAddress BufferAddress() { return m_DeviceAddress; }
    size_t          Size() { return m_Size; }

    uint32_t AllocateSSBO(const BufferHandle& buffer);
    uint32_t AllocateUBO(const BufferHandle& buffer);
    uint32_t AllocateTexture(const TextureHandle& handle);

private:
    size_t          m_Size          = 0;
    VkBuffer        m_Buffer        = VK_NULL_HANDLE;
    VkDeviceMemory  m_DeviceMemory  = VK_NULL_HANDLE;
    VkDeviceAddress m_DeviceAddress = 0;

    //
    size_t              m_CurrentOffset = 0;
    std::vector<size_t> m_FreeList;
};
} // namespace Lgt::Gpu