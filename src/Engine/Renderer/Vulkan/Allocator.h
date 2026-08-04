#pragma once
#include "Helpers.h"

class VulkanAllocator {
public:
    VulkanAllocator(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device);
    ~VulkanAllocator();

    bool createBuffer(VkDeviceSize             size,
                      VkBufferUsageFlags       usage,
                      VmaMemoryUsage           memoryUsage,
                      VmaAllocationCreateFlags flags,
                      VkBuffer&                outBuffer,
                      VmaAllocation&           outAllocation,
                      VmaAllocationInfo*       outInfo = nullptr);

    bool createBuffer(const VkBufferCreateInfo&      bufferInfo,
                      const VmaAllocationCreateInfo& allocInfo,
                      VkBuffer&                      outBuffer,
                      VmaAllocation&                 outAllocation,
                      VmaAllocationInfo*             outInfo = nullptr);

    bool createBufferWithAlignment(const VkBufferCreateInfo&      bufferInfo,
                                   const VmaAllocationCreateInfo& allocInfo,
                                   VkDeviceSize                   minAlignment,
                                   VkBuffer&                      outBuffer,
                                   VmaAllocation&                 outAllocation,
                                   VmaAllocationInfo*             outInfo = nullptr);

    void destroyBuffer(VkBuffer buffer, VmaAllocation allocation);

    bool createImage(const VkImageCreateInfo& imageInfo,
                     VmaMemoryUsage           memoryUsage,
                     VmaAllocationCreateFlags flags,
                     VkImage&                 outImage,
                     VmaAllocation&           outAllocation,
                     VmaAllocationInfo*       outInfo = nullptr);

    void         destroyImage(VkImage image, VmaAllocation allocation);
    void*        map(VmaAllocation allocation);
    void         unmap(VmaAllocation allocation);
    VmaAllocator Handle() const { return m_Allocator; }

private:
    VmaAllocator m_Allocator = VK_NULL_HANDLE;
};