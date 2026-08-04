#pragma once

#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <vector>

#include <volk.h>
#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"

// -----------------------------------------------------------------------
//  Constants
// -----------------------------------------------------------------------
static constexpr uint32_t WIDTH                = 800;
static constexpr uint32_t HEIGHT               = 600;
static constexpr uint32_t MAX_RESOURCES        = 1024;
static constexpr uint32_t MAX_SAMPLERS         = 32;
static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 3;

static const std::vector<const char*> VALIDATION_LAYERS = {"VK_LAYER_KHRONOS_validation"};

static const std::vector<const char*> DEVICE_EXTENSIONS = {VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                                                           VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
                                                           VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
                                                           VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
                                                           VK_KHR_MAINTENANCE_5_EXTENSION_NAME,
                                                           VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME,
                                                           VK_EXT_DESCRIPTOR_HEAP_EXTENSION_NAME,
                                                           VK_KHR_SHADER_UNTYPED_POINTERS_EXTENSION_NAME};

#ifdef NDEBUG
static constexpr bool ENABLE_VALIDATION = false;
#else
static constexpr bool ENABLE_VALIDATION = true;
#endif

// forward decals
class VulkanDevice;

//  Supporting structs
struct QueueFamilyIndices {
    std::optional<uint32_t> graphics;
    std::optional<uint32_t> present;

    bool isComplete() const { return graphics.has_value() && present.has_value(); }
};

//  Free-standing helpers
constexpr uint64_t AlignUp(uint64_t value, uint64_t alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

/// Load a SPIR-V binary from disk.
inline std::vector<char> readFile(const std::string& path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error("Failed to open shader: " + path);

    const size_t      size = static_cast<size_t>(file.tellg());
    std::vector<char> buf(size);
    file.seekg(0);
    file.read(buf.data(), static_cast<std::streamsize>(size));
    return buf;
}

/// Vulkan debug-utils callback — forwards warnings/errors to stderr.
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                                    VkDebugUtilsMessageTypeFlagsEXT /*type*/,
                                                    const VkDebugUtilsMessengerCallbackDataEXT* data,
                                                    void* /*userdata*/) {
    if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        std::cerr << "[Validation] " << data->pMessage << '\n';
    return VK_FALSE;
}

VkShaderModule  createShaderModule(VulkanDevice* device, const std::vector<char>& code);
VkDeviceAddress getBufferDeviceAddress(const VkBuffer& buffer);
uint32_t        selectMemoryType(VulkanDevice* device, uint32_t typeFilter, VkMemoryPropertyFlags flags);
