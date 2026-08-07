#include "Context.h"
#include "Engine/Core/Logger.h"
#include "Engine/Core/VkCheck.h"
#include "Uploader.h"

namespace Lgt::Vulkan {

VulkanLoadTimeStagingUploader::VulkanLoadTimeStagingUploader() {

    auto     device         = g_Device->Logical();
    uint32_t graphicsFamily = g_Device->GraphicsFamily();

    // Use the graphics queue because the final image barrier targets shader access.
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = graphicsFamily;
    poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CHECK(vkCreateCommandPool(device, &poolInfo, nullptr, &m_Pool));

    // Single command buffer — reused between Flush() calls
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool        = m_Pool;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    VK_CHECK(vkAllocateCommandBuffers(device, &allocInfo, &m_Cmd));

    // Fence starts unsignaled — Flush() submits then waits on it
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    VK_CHECK(vkCreateFence(device, &fenceInfo, nullptr, &m_Fence));
}

VulkanLoadTimeStagingUploader::~VulkanLoadTimeStagingUploader() {
    auto device = g_Device->Logical();

    if (m_Fence != VK_NULL_HANDLE)
        vkDestroyFence(device, m_Fence, nullptr);

    if (m_Pool != VK_NULL_HANDLE)
        vkDestroyCommandPool(device, m_Pool, nullptr);
    // m_Cmd is owned by m_Pool, destroyed implicitly
}

void VulkanLoadTimeStagingUploader::UploadBuffer(VkBuffer dst, const void* data, uint32_t size, uint32_t dstOffset) {

    LGT_ASSERT(dst != VK_NULL_HANDLE, "[LoadUploader] dst buffer is null");
    LGT_ASSERT(data, "[LoadUploader] data pointer is null");
    LGT_ASSERT(size > 0, "[LoadUploader] upload size is 0");

    uint32_t offset = pushData(data, size);
    m_BufferUploads.push_back({dst, offset, dstOffset, size});
    m_TotalSize += size;
}

void VulkanLoadTimeStagingUploader::uploadTexture(VkImage dst, const void* data, uint32_t size, const TextureCopy& region) {

    LGT_ASSERT(dst != VK_NULL_HANDLE, "[LoadUploader] dst image is null");
    LGT_ASSERT(data, "[LoadUploader] data pointer is null");
    LGT_ASSERT(size > 0, "[LoadUploader] upload size is 0");

    uint32_t offset = pushData(data, size);
    m_TextureUploads.push_back({dst, offset, size, region});
    m_TotalSize += size;
}

void VulkanLoadTimeStagingUploader::Flush() {
    if (m_BufferUploads.empty() && m_TextureUploads.empty())
        return;

    LIGHTVK_INFO("[LoadUploader] Flushing {} buffer(s) and {} texture(s) ({:.2f} MB)",
                 m_BufferUploads.size(),
                 m_TextureUploads.size(),
                 m_CPUData.size() / (1024.0f * 1024.0f));

    VkBuffer          stagingBuffer = VK_NULL_HANDLE;
    VmaAllocation     stagingAlloc  = VK_NULL_HANDLE;
    VmaAllocationInfo stagingInfo{};

    VmaAllocationCreateInfo vmaCI{};
    vmaCI.usage = VMA_MEMORY_USAGE_AUTO;
    vmaCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    bool ok = g_Allocator->createBuffer((uint32_t)m_CPUData.size(),
                                                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                VMA_MEMORY_USAGE_AUTO,
                                                vmaCI.flags,
                                                stagingBuffer,
                                                stagingAlloc,
                                                &stagingInfo);

    LGT_ASSERT(ok, "Failed to create staging buffer ({:.2f} MB)", m_CPUData.size() / (1024.0f * 1024.0f));

    // Single memcpy — all data already packed in m_CPUData
    std::memcpy(stagingInfo.pMappedData, m_CPUData.data(), m_CPUData.size());

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VK_CHECK(vkBeginCommandBuffer(m_Cmd, &beginInfo));

    if (!m_TextureUploads.empty()) {
        std::vector<VkImageMemoryBarrier> preBarriers;
        preBarriers.reserve(m_TextureUploads.size());

        for (auto& up : m_TextureUploads) {
            VkImageMemoryBarrier b{};
            b.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            b.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
            b.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            b.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            b.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            b.image                           = up.dst;
            b.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            b.subresourceRange.baseMipLevel   = up.region.dstMipLevel;
            b.subresourceRange.levelCount     = 1;
            b.subresourceRange.baseArrayLayer = up.region.dstArrayLayer;
            b.subresourceRange.layerCount     = 1;
            b.srcAccessMask                   = 0;
            b.dstAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;
            preBarriers.push_back(b);
        }

        vkCmdPipelineBarrier(m_Cmd,
                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0,
                             0,
                             nullptr,
                             0,
                             nullptr,
                             (uint32_t)preBarriers.size(),
                             preBarriers.data());
    }

    for (auto& up : m_BufferUploads) {
        VkBufferCopy region{};
        region.srcOffset = up.stagingOffset;
        region.dstOffset = up.dstOffset;
        region.size      = up.size;
        vkCmdCopyBuffer(m_Cmd, stagingBuffer, up.dst, 1, &region);
    }

    for (auto& up : m_TextureUploads) {
        VkBufferImageCopy region{};
        region.bufferOffset                    = up.stagingOffset;
        region.bufferRowLength                 = 0; // tightly packed
        region.bufferImageHeight               = 0;
        region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel       = up.region.dstMipLevel;
        region.imageSubresource.baseArrayLayer = up.region.dstArrayLayer;
        region.imageSubresource.layerCount     = 1;
        region.imageOffset = {(int32_t)up.region.dstOffset.x, (int32_t)up.region.dstOffset.y, (int32_t)up.region.dstOffset.z};
        region.imageExtent = {(uint32_t)up.region.extent.x, (uint32_t)up.region.extent.y, (uint32_t)up.region.extent.z};
        vkCmdCopyBufferToImage(m_Cmd, stagingBuffer, up.dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    }

    if (!m_TextureUploads.empty()) {
        std::vector<VkImageMemoryBarrier> postBarriers;
        postBarriers.reserve(m_TextureUploads.size());

        for (auto& up : m_TextureUploads) {
            VkImageMemoryBarrier b{};
            b.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            b.oldLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            b.newLayout                       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            b.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            b.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            b.image                           = up.dst;
            b.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            b.subresourceRange.baseMipLevel   = up.region.dstMipLevel;
            b.subresourceRange.levelCount     = 1;
            b.subresourceRange.baseArrayLayer = up.region.dstArrayLayer;
            b.subresourceRange.layerCount     = 1;
            b.srcAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;
            b.dstAccessMask                   = VK_ACCESS_SHADER_READ_BIT;
            postBarriers.push_back(b);
        }

        vkCmdPipelineBarrier(m_Cmd,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                             0,
                             0,
                             nullptr,
                             0,
                             nullptr,
                             (uint32_t)postBarriers.size(),
                             postBarriers.data());
    }

    VK_CHECK(vkEndCommandBuffer(m_Cmd));

    VkSubmitInfo submitInfo{};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &m_Cmd;

    VK_CHECK(vkQueueSubmit(g_Device->GraphicsQueue(), 1, &submitInfo, m_Fence));
    VK_CHECK(vkWaitForFences(g_Device->Logical(), 1, &m_Fence, VK_TRUE, UINT64_MAX));
    VK_CHECK(vkResetFences(g_Device->Logical(), 1, &m_Fence));

    g_Allocator->destroyBuffer(stagingBuffer, stagingAlloc);

    LIGHTVK_INFO("[LoadUploader] Flush complete -> staging memory freed");

    m_CPUData.clear();
    m_BufferUploads.clear();
    m_TextureUploads.clear();
    m_TotalSize = 0;

    vkResetCommandBuffer(m_Cmd, 0);
}

uint32_t VulkanLoadTimeStagingUploader::pushData(const void* data, uint32_t size, uint32_t alignment) {
    // Pad to alignment boundary
    uint32_t offset = (uint32_t)((m_CPUData.size() + alignment - 1) & ~(alignment - 1));
    m_CPUData.resize(offset + size);
    std::memcpy(m_CPUData.data() + offset, data, size);
    return offset;
}

} // namespace Lgt::Vulkan
