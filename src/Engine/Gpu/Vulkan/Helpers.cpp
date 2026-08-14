
#define VMA_IMPLEMENTATION
#include "Helpers.h"

#include "Context.h"
#include "Engine/Gpu/Context.h"

uint32_t selectMemoryType(VulkanDevice* device, uint32_t typeFilter, VkMemoryPropertyFlags flags) {
    auto deviceprops = device->PhysicalDeviceMemoryProperties();
    for (int i = 0; i < deviceprops.memoryTypeCount; ++i) {
        if (typeFilter & (1 << i) && (flags & deviceprops.memoryTypes[i].propertyFlags) == flags)
            return i;
    }

    LIGHTVK_CRITICAL("Cannot find a sutiable memory type for the buffer");
    return UINT32_MAX;
}

VkShaderModule createShaderModule(VulkanDevice* device, const std::vector<char>& code) {
    VkShaderModuleCreateInfo ci{};
    ci.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ci.codeSize = code.size();
    ci.pCode    = reinterpret_cast<const uint32_t*>(code.data());
    // ci.flags    = VK_SHADER_CREATE_DESCRIPTOR_HEAP_BIT_EXT;
    VkShaderModule mod;
    if (vkCreateShaderModule(device->Logical(), &ci, nullptr, &mod) != VK_SUCCESS)
        throw std::runtime_error("vkCreateShaderModule failed");
    return mod;
}

VkDeviceAddress getBufferDeviceAddress(const VkBuffer& buffer) {
    VkBufferDeviceAddressInfo adderInfo{};
    adderInfo.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    adderInfo.buffer = buffer;
    return vkGetBufferDeviceAddress(Lgt::Vulkan::g_Device->Logical(), &adderInfo);
}
