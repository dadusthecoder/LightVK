#include "DescriptorHeap.h"
#include "Context.h"
#include "Engine/Renderer/Vulkan/Context.h"
#include "Engine/Core/Logger.h"
#include "Engine/Core/VkCheck.h"

namespace Lgt::Gpu {

DescriptorHeap::DescriptorHeap(size_t size, bool isResorceHeap, bool isSamplerHeap) {

    m_Size = size;

    VkBufferCreateInfo bufferci{};
    bufferci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferci.size  = size;
    bufferci.usage = VK_BUFFER_USAGE_DESCRIPTOR_HEAP_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    vkCreateBuffer(Lgt::Vulkan::g_Device->Logical(), &bufferci, nullptr, &m_Buffer);

    VkMemoryRequirements memReu{};
    vkGetBufferMemoryRequirements(Lgt::Vulkan::g_Device->Logical(), m_Buffer, &memReu);

    VkMemoryAllocateInfo allocateinfo{};
    allocateinfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateinfo.allocationSize  = memReu.size;
    allocateinfo.memoryTypeIndex = selectMemoryType(Lgt::Vulkan::g_Device,
                                                    memReu.memoryTypeBits,
                                                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    VkMemoryAllocateFlagsInfo flagsinfo{};
    flagsinfo.sType      = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
    flagsinfo.deviceMask = 1;
    flagsinfo.flags      = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

    // chaining
    allocateinfo.pNext = &flagsinfo;
    VK_CHECK(vkAllocateMemory(Lgt::Vulkan::g_Device->Logical(), &allocateinfo, nullptr, &m_DeviceMemory));
    VK_CHECK(vkBindBufferMemory(Lgt::Vulkan::g_Device->Logical(), m_Buffer, m_DeviceMemory, 0));

    VkBufferDeviceAddressInfo addressinfo{};
    addressinfo.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    addressinfo.buffer = m_Buffer;
    m_DeviceAddress    = vkGetBufferDeviceAddress(Lgt::Vulkan::g_Device->Logical(), &addressinfo);
    LIGHTVK_INFO("Created DescriptorHeap -size : {} bytes -BufferDeviceAdder : {}", m_Size, m_DeviceAddress);
}

DescriptorHeap::~DescriptorHeap() {
    if (m_Buffer != VK_NULL_HANDLE)
        vkDestroyBuffer(Lgt::Vulkan::g_Device->Logical(), m_Buffer, nullptr);

    if (m_DeviceMemory != VK_NULL_HANDLE)
        vkFreeMemory(Lgt::Vulkan::g_Device->Logical(), m_DeviceMemory, nullptr);
}

uint32_t DescriptorHeap::AllocateSSBO(const BufferHandle& buffer) {
    m_CurrentOffset   = AlignUp(m_CurrentOffset, Vulkan::g_Device->DescriptorHeapProperties().bufferDescriptorAlignment);
    uint32_t gpuIndex = m_CurrentOffset / Vulkan::g_Device->DescriptorHeapProperties().bufferDescriptorSize;
    LGT_ASSERT(m_CurrentOffset + Vulkan::g_Device->DescriptorHeapProperties().bufferDescriptorSize < m_Size,
               "Heap OverFlow");

    auto* gpubuffer = g_Buffers.Get(buffer);
    LGT_ASSERT(gpubuffer, "");

    VkDeviceAddressRangeEXT deviceAdderRange{};
    deviceAdderRange.size = gpubuffer->size;
    LGT_ASSERT(gpubuffer->deviceAddress % Vulkan::g_Device->Limits().minStorageBufferOffsetAlignment == 0,
               "data->pAddressRange→address must be a multiple of minStorageBufferOffsetAlignment");
    deviceAdderRange.address = gpubuffer->deviceAddress;

    VkResourceDescriptorDataEXT descData{};
    descData.pAddressRange = &deviceAdderRange;

    VkResourceDescriptorInfoEXT descInfo{};
    descInfo.sType = VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT;
    descInfo.type  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descInfo.data  = descData;

    void* ptr;
    vkMapMemory(Vulkan::g_Device->Logical(), m_DeviceMemory, 0, m_Size, 0, &ptr);

    VkHostAddressRangeEXT hostRange{};
    hostRange.address = (uint8_t*)ptr + m_CurrentOffset;
    hostRange.size    = Vulkan::g_Device->DescriptorHeapProperties().bufferDescriptorSize;

    vkWriteResourceDescriptorsEXT(Vulkan::g_Device->Logical(), 1, &descInfo, &hostRange);
    vkUnmapMemory(Vulkan::g_Device->Logical(), m_DeviceMemory);

    m_CurrentOffset += Vulkan::g_Device->DescriptorHeapProperties().bufferDescriptorSize;

    LIGHTVK_INFO("DescriptorHeap : Allocated SSBO ,  GPU index : {}", gpuIndex);
    return gpuIndex;
}

uint32_t DescriptorHeap::AllocateUBO(const BufferHandle& buffer) {
    m_CurrentOffset   = AlignUp(m_CurrentOffset, Vulkan::g_Device->DescriptorHeapProperties().bufferDescriptorAlignment);
    uint32_t gpuIndex = m_CurrentOffset / Vulkan::g_Device->DescriptorHeapProperties().bufferDescriptorSize;
    LGT_ASSERT(m_CurrentOffset + Vulkan::g_Device->DescriptorHeapProperties().bufferDescriptorSize < m_Size,
               "Heap OverFlow");

    auto* gpubuffer = g_Buffers.Get(buffer);
    LGT_ASSERT(gpubuffer, "");

    VkDeviceAddressRangeEXT deviceAdderRange{};
    deviceAdderRange.size = gpubuffer->size;
    LGT_ASSERT(gpubuffer->deviceAddress % Vulkan::g_Device->Limits().minUniformBufferOffsetAlignment == 0,
               "data->pAddressRange→address must be a multiple of minUniformBufferOffsetAlignment");
    deviceAdderRange.address = gpubuffer->deviceAddress;

    VkResourceDescriptorDataEXT descData{};
    descData.pAddressRange = &deviceAdderRange;

    VkResourceDescriptorInfoEXT descInfo{};
    descInfo.sType = VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT;
    descInfo.type  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descInfo.data  = descData;

    void* ptr;
    vkMapMemory(Vulkan::g_Device->Logical(), m_DeviceMemory, 0, m_Size, 0, &ptr);

    VkHostAddressRangeEXT hostRange{};
    hostRange.address = (uint8_t*)ptr + m_CurrentOffset;
    hostRange.size    = Vulkan::g_Device->DescriptorHeapProperties().bufferDescriptorSize;

    vkWriteResourceDescriptorsEXT(Vulkan::g_Device->Logical(), 1, &descInfo, &hostRange);
    vkUnmapMemory(Vulkan::g_Device->Logical(), m_DeviceMemory);

    m_CurrentOffset += Vulkan::g_Device->DescriptorHeapProperties().bufferDescriptorSize;

    LIGHTVK_INFO("DescriptorHeap : Allocated UBO ,  GPU index : {}", gpuIndex);
    return gpuIndex;
}

} // namespace Lgt::Gpu