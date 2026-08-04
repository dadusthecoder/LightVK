#include "Engine/Core/Logger.h"
#include "Engine/Core/VkCheck.h"
#include "Allocator.h"

VulkanAllocator::VulkanAllocator(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device) {
    VmaAllocatorCreateInfo info{};

    VmaVulkanFunctions functions{};
    functions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    functions.vkGetDeviceProcAddr   = vkGetDeviceProcAddr;

    info.instance         = instance;
    info.physicalDevice   = physicalDevice;
    info.device           = device;
    info.pVulkanFunctions = &functions;
    info.flags            = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT | VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

    VK_CHECK(vmaCreateAllocator(&info, &m_Allocator));
}

VulkanAllocator::~VulkanAllocator() {
    if (m_Allocator)
        vmaDestroyAllocator(m_Allocator);
}

bool VulkanAllocator::createBuffer(VkDeviceSize             size,
                                   VkBufferUsageFlags       usage,
                                   VmaMemoryUsage           memoryUsage,
                                   VmaAllocationCreateFlags flags,
                                   VkBuffer&                outBuffer,
                                   VmaAllocation&           outAllocation,
                                   VmaAllocationInfo*       outInfo) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size        = size;
    bufferInfo.usage       = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = memoryUsage;
    allocInfo.flags = flags;

    VkResult res = vmaCreateBuffer(m_Allocator, &bufferInfo, &allocInfo, &outBuffer, &outAllocation, outInfo);
    VK_CHECK(res);
    return res == VK_SUCCESS;
}

bool VulkanAllocator::createBuffer(const VkBufferCreateInfo&      bufferInfo,
                                   const VmaAllocationCreateInfo& allocInfo,
                                   VkBuffer&                      outBuffer,
                                   VmaAllocation&                 outAllocation,
                                   VmaAllocationInfo*             outInfo) {

    VkResult res = vmaCreateBuffer(m_Allocator, &bufferInfo, &allocInfo, &outBuffer, &outAllocation, outInfo);
    VK_CHECK(res);
    return res == VK_SUCCESS;
}

bool VulkanAllocator::createBufferWithAlignment(const VkBufferCreateInfo&      bufferInfo,
                                                const VmaAllocationCreateInfo& allocInfo,
                                                VkDeviceSize                   minAlignment,
                                                VkBuffer&                      outBuffer,
                                                VmaAllocation&                 outAllocation,
                                                VmaAllocationInfo*             outInfo) {
    VkResult res = vmaCreateBufferWithAlignment(m_Allocator, &bufferInfo, &allocInfo, minAlignment, &outBuffer, &outAllocation, outInfo);
    VK_CHECK(res);
    return res == VK_SUCCESS;
}

void VulkanAllocator::destroyBuffer(VkBuffer buffer, VmaAllocation allocation) {
    vmaDestroyBuffer(m_Allocator, buffer, allocation);
}

bool VulkanAllocator::createImage(const VkImageCreateInfo& imageInfo,
                                  VmaMemoryUsage           memoryUsage,
                                  VmaAllocationCreateFlags flags,
                                  VkImage&                 outImage,
                                  VmaAllocation&           outAllocation,
                                  VmaAllocationInfo*       outInfo) {
    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = memoryUsage;
    allocInfo.flags = flags;

    VkResult res = vmaCreateImage(m_Allocator, &imageInfo, &allocInfo, &outImage, &outAllocation, outInfo);

    return res == VK_SUCCESS;
}

void VulkanAllocator::destroyImage(VkImage image, VmaAllocation allocation) {
    vmaDestroyImage(m_Allocator, image, allocation);
}

void* VulkanAllocator::map(VmaAllocation allocation) {
    void* ptr = nullptr;
    vmaMapMemory(m_Allocator, allocation, &ptr);
    return ptr;
}

void VulkanAllocator::unmap(VmaAllocation allocation) {
    vmaUnmapMemory(m_Allocator, allocation);
}
