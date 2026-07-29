#include "Resource.h"
#include "Context.h"
#include "Engine/Renderer/Vulkan/Context.h"

namespace Lgt::Gpu {
BufferHandle CreateSSBO(size_t size, bool dynamic) {
    VkBufferCreateInfo bufferci{};
    bufferci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferci.size  = size;

    if (!dynamic)
        bufferci.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    bufferci.usage       |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    bufferci.sharingMode  = Vulkan::g_Device->GraphicsFamily() == Vulkan::g_Device->TransferFamily()
                                ? VK_SHARING_MODE_CONCURRENT
                                : VK_SHARING_MODE_EXCLUSIVE;

    uint32_t queuefamilyindices[]  = {Vulkan::g_Device->GraphicsFamily(), Vulkan::g_Device->TransferFamily()};
    bufferci.queueFamilyIndexCount = 2;
    bufferci.pQueueFamilyIndices   = queuefamilyindices;

    Buffer                  buffer{};
    VmaAllocationCreateInfo allocInfo{};

    if (dynamic) {
        allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    } else {
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    }

    if (Vulkan::g_Allocator->createBuffer(bufferci, allocInfo, buffer.buffer, buffer.allocation)) {
        buffer.deviceAddress = getBufferDeviceAddress(buffer.buffer);
        buffer.size          = size;
        LIGHTVK_INFO("Created SSBO size: {}", size);
        return g_Buffers.Allocate(buffer);
    }

    LIGHTVK_CRITICAL("Failed to create SSBO");
    return BufferHandle();
}

BufferHandle CreateUBO(size_t size) {
    VkBufferCreateInfo bufferci{};
    bufferci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferci.size  = size;

    bufferci.usage       = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    bufferci.sharingMode = Vulkan::g_Device->GraphicsFamily() == Vulkan::g_Device->TransferFamily()
                               ? VK_SHARING_MODE_CONCURRENT
                               : VK_SHARING_MODE_EXCLUSIVE;

    uint32_t queuefamilyindices[]  = {Vulkan::g_Device->GraphicsFamily(), Vulkan::g_Device->TransferFamily()};
    bufferci.queueFamilyIndexCount = 2;
    bufferci.pQueueFamilyIndices   = queuefamilyindices;

    Buffer                  buffer{};
    VmaAllocationCreateInfo allocInfo{};

    allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    if (Vulkan::g_Allocator->createBuffer(bufferci, allocInfo, buffer.buffer, buffer.allocation)) {
        buffer.deviceAddress = getBufferDeviceAddress(buffer.buffer);
        buffer.size          = size;
        buffer.mapped        = Vulkan::g_Allocator->map(buffer.allocation);
        LIGHTVK_INFO("Created UBO size: {}", size);
        return g_Buffers.Allocate(buffer);
    }

    LIGHTVK_CRITICAL("Failed to create UBO");
    return BufferHandle();
}

} // namespace Lgt::Gpu
