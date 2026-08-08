#include "DescriptorHeap.h"
#include "Context.h"
#include "Engine/Renderer/Vulkan/Context.h"
#include "Engine/Core/Logger.h"
#include "Engine/Core/VkCheck.h"

namespace Lgt::Gpu {

DescriptorHeap::DescriptorHeap(size_t size, bool isResorceHeap, bool isSamplerHeap) {

    VkBufferCreateInfo bufferci{};

    bufferci.usage = VK_BUFFER_USAGE_DESCRIPTOR_HEAP_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    if (isResorceHeap) {
        bufferci.usage       |= VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;
        _reserved_range_size  = Vulkan::g_Device->DescriptorHeapProperties().minResourceHeapReservedRange;
    } else if (isSamplerHeap) {
        bufferci.usage       |= VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT;
        _reserved_range_size  = Vulkan::g_Device->DescriptorHeapProperties().minSamplerHeapReservedRange;
    }

    _usable_size           = size;
    _total_size            = _usable_size + _reserved_range_size;
    _reserved_range_offset = _total_size - _reserved_range_size;

    bufferci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferci.size  = _total_size;

    vkCreateBuffer(Lgt::Vulkan::g_Device->Logical(), &bufferci, nullptr, &m_Buffer);

    VkMemoryRequirements memReu{};
    vkGetBufferMemoryRequirements(Lgt::Vulkan::g_Device->Logical(), m_Buffer, &memReu);

    VkMemoryAllocateInfo allocateinfo{};
    allocateinfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateinfo.allocationSize  = memReu.size;
    allocateinfo.memoryTypeIndex = selectMemoryType(
        Lgt::Vulkan::g_Device, memReu.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

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

    m_DeviceAddress = vkGetBufferDeviceAddress(Lgt::Vulkan::g_Device->Logical(), &addressinfo);

    LIGHTVK_INFO("Created , total -size : {} MB -BufferDeviceAdder : {}", (float)_total_size / 1024, m_DeviceAddress);
    LIGHTVK_INFO(
        "Usable size : {} MB  ,ReservedRange size : {} MB", (float)_usable_size / 1024, (float)_reserved_range_size / 1024);

    _texture_offset = AlignUp((_usable_size / 100) * 25, Vulkan::g_Device->DescriptorHeapProperties().imageDescriptorAlignment);
    _sampler_offset = AlignUp(_sampler_offset, Vulkan::g_Device->DescriptorHeapProperties().samplerDescriptorAlignment);
    _buffer_offset  = AlignUp(_buffer_offset, Vulkan::g_Device->DescriptorHeapProperties().bufferDescriptorAlignment);
}

DescriptorHeap::~DescriptorHeap() {
    if (m_Buffer != VK_NULL_HANDLE)
        vkDestroyBuffer(Lgt::Vulkan::g_Device->Logical(), m_Buffer, nullptr);

    if (m_DeviceMemory != VK_NULL_HANDLE)
        vkFreeMemory(Lgt::Vulkan::g_Device->Logical(), m_DeviceMemory, nullptr);
}

uint32_t DescriptorHeap::AllocateSSBO(const BufferHandle& buffer) {

    uint32_t gpuIndex = _buffer_offset / Vulkan::g_Device->DescriptorHeapProperties().bufferDescriptorSize;
    LGT_ASSERT(_buffer_offset + Vulkan::g_Device->DescriptorHeapProperties().bufferDescriptorSize < _texture_offset,
               "BufferDescriptor Heap OverFlow");

    auto* gpubuffer = Resources->GetBuffer(buffer);
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
    vkMapMemory(Vulkan::g_Device->Logical(), m_DeviceMemory, 0, _total_size, 0, &ptr);

    if ((uint64_t)ptr % Vulkan::g_Device->DescriptorHeapProperties().resourceHeapAlignment != 0) {
        LIGHTVK_WARN(
            "ResourceHeap's start must be aligned to Vulkan::g_Device->DescriptorHeapProperties().resourceHeapAlignment ");
        ptr = (void*)AlignUp((uint64_t)ptr, Vulkan::g_Device->DescriptorHeapProperties().resourceHeapAlignment);
    }

    VkHostAddressRangeEXT hostRange{};
    hostRange.address = (uint8_t*)ptr + _buffer_offset;
    hostRange.size    = Vulkan::g_Device->DescriptorHeapProperties().bufferDescriptorSize;

    vkWriteResourceDescriptorsEXT(Vulkan::g_Device->Logical(), 1, &descInfo, &hostRange);
    vkUnmapMemory(Vulkan::g_Device->Logical(), m_DeviceMemory);

    LIGHTVK_INFO("Allocated at offset {}  GPU index : {}", _buffer_offset, gpuIndex);

    _buffer_offset += Vulkan::g_Device->DescriptorHeapProperties().bufferDescriptorSize;

    return gpuIndex;
}

uint32_t DescriptorHeap::AllocateUBO(const BufferHandle& buffer) {

    uint32_t gpuIndex = _buffer_offset / Vulkan::g_Device->DescriptorHeapProperties().bufferDescriptorSize;
    LGT_ASSERT(_buffer_offset + Vulkan::g_Device->DescriptorHeapProperties().bufferDescriptorSize < _texture_offset,
               "Bufferdescriptor Heap OverFlow");

    auto* gpubuffer = Resources->GetBuffer(buffer);
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
    vkMapMemory(Vulkan::g_Device->Logical(), m_DeviceMemory, 0, _total_size, 0, &ptr);

    if ((uint64_t)ptr % Vulkan::g_Device->DescriptorHeapProperties().resourceHeapAlignment != 0) {
        LIGHTVK_WARN(
            "ResourceHeap's start must be aligned to Vulkan::g_Device->DescriptorHeapProperties().resourceHeapAlignment ");
        ptr = (void*)AlignUp((uint64_t)ptr, Vulkan::g_Device->DescriptorHeapProperties().resourceHeapAlignment);
    }

    VkHostAddressRangeEXT hostRange{};
    hostRange.address = (uint8_t*)ptr + _buffer_offset;
    hostRange.size    = Vulkan::g_Device->DescriptorHeapProperties().bufferDescriptorSize;

    vkWriteResourceDescriptorsEXT(Vulkan::g_Device->Logical(), 1, &descInfo, &hostRange);
    vkUnmapMemory(Vulkan::g_Device->Logical(), m_DeviceMemory);

    LIGHTVK_INFO("Allocated at offset {}  GPU index : {}", _buffer_offset, gpuIndex);
    _buffer_offset += Vulkan::g_Device->DescriptorHeapProperties().bufferDescriptorSize;

    return gpuIndex;
}

uint32_t DescriptorHeap::AllocateTexture(const TextureHandle& handle, const VkImageViewCreateInfo& view, VkImageLayout layout) {

    uint32_t gpuIndex = _texture_offset / Vulkan::g_Device->DescriptorHeapProperties().imageDescriptorSize;
    LGT_ASSERT(_texture_offset + Vulkan::g_Device->DescriptorHeapProperties().imageDescriptorSize < _usable_size,
               "Heap OverFlow");

    VkImageDescriptorInfoEXT imageInfo{};
    imageInfo.sType  = VK_STRUCTURE_TYPE_IMAGE_DESCRIPTOR_INFO_EXT;
    imageInfo.layout = layout;
    imageInfo.pView  = &view;

    VkResourceDescriptorDataEXT descData{};
    descData.pImage = &imageInfo;

    VkResourceDescriptorInfoEXT descInfo{};
    descInfo.sType = VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT;
    descInfo.type  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    descInfo.data  = descData;

    void* ptr;

    vkMapMemory(Vulkan::g_Device->Logical(), m_DeviceMemory, 0, _total_size, 0, &ptr);

    if ((uint64_t)ptr % Vulkan::g_Device->DescriptorHeapProperties().resourceHeapAlignment != 0) {
        LIGHTVK_WARN(
            "ResourceHeap's start must be aligned to Vulkan::g_Device->DescriptorHeapProperties().resourceHeapAlignment ");
        ptr = (void*)AlignUp((uint64_t)ptr, Vulkan::g_Device->DescriptorHeapProperties().resourceHeapAlignment);
    }

    VkHostAddressRangeEXT hostRange{};
    hostRange.address = (uint8_t*)ptr + _texture_offset;
    hostRange.size    = Vulkan::g_Device->DescriptorHeapProperties().imageDescriptorSize;

    vkWriteResourceDescriptorsEXT(Vulkan::g_Device->Logical(), 1, &descInfo, &hostRange);

    vkUnmapMemory(Vulkan::g_Device->Logical(), m_DeviceMemory);

    LIGHTVK_INFO("Allocated at offset {}  GPU index : {}", _texture_offset, gpuIndex);
    _texture_offset += Vulkan::g_Device->DescriptorHeapProperties().imageDescriptorSize;

    return gpuIndex;
}

uint32_t DescriptorHeap::AllocateSampler(const VkSamplerCreateInfo& samplerInfo) {
    const auto& properties = Vulkan::g_Device->DescriptorHeapProperties();

    uint32_t gpuIndex = static_cast<uint32_t>(_sampler_offset / properties.samplerDescriptorSize);
    LGT_ASSERT(_sampler_offset + properties.samplerDescriptorSize <= _usable_size, "Sampler heap overflow");

    void* ptr = nullptr;
    VK_CHECK(vkMapMemory(Vulkan::g_Device->Logical(), m_DeviceMemory, 0, _total_size, 0, &ptr));

    if ((uint64_t)ptr % Vulkan::g_Device->DescriptorHeapProperties().samplerHeapAlignment != 0) {
        LIGHTVK_WARN(
            "ResourceHeap's start must be aligned to Vulkan::g_Device->DescriptorHeapProperties().samplerHeapAlignment ");
        ptr = (void*)AlignUp((uint64_t)ptr, Vulkan::g_Device->DescriptorHeapProperties().samplerHeapAlignment);
    }

    VkHostAddressRangeEXT hostRange{};
    hostRange.address = static_cast<uint8_t*>(ptr) + _sampler_offset;
    hostRange.size    = properties.samplerDescriptorSize;

    VK_CHECK(vkWriteSamplerDescriptorsEXT(Vulkan::g_Device->Logical(), 1, &samplerInfo, &hostRange));
    vkUnmapMemory(Vulkan::g_Device->Logical(), m_DeviceMemory);

    LIGHTVK_INFO("Allocated at offset {}  GPU index : {}", _sampler_offset, gpuIndex);
    _sampler_offset += properties.samplerDescriptorSize;

    return gpuIndex;
}

} // namespace Lgt::Gpu
