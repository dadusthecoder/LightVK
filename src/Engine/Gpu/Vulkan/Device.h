#pragma once
#include <vector>
#include "Helpers.h"

struct DeviceInfo {
    VkPhysicalDevice                     device;
    VkPhysicalDeviceProperties           properties;
    VkPhysicalDeviceFeatures             features;
    VkPhysicalDeviceMemoryProperties     memoryProperties;
    std::vector<VkQueueFamilyProperties> queueFamilies;
    std::vector<VkExtensionProperties>   extensions;
    uint32_t                             score = 0;
    int                                  index = 0;
};

struct DeviceFeatureChain {
    VkPhysicalDeviceFeatures2                        core{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
    VkPhysicalDeviceVulkan11Features                 vk11{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};
    VkPhysicalDeviceVulkan12Features                 vk12{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    VkPhysicalDeviceVulkan13Features                 vk13{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    VkPhysicalDeviceDescriptorHeapFeaturesEXT        descHeap{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_HEAP_FEATURES_EXT};
    VkPhysicalDeviceShaderUntypedPointersFeaturesKHR untyped{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_UNTYPED_POINTERS_FEATURES_KHR};
    DeviceFeatureChain() {
        core.pNext     = &vk11;
        vk11.pNext     = &vk12;
        vk12.pNext     = &vk13;
        vk13.pNext     = &descHeap;
        descHeap.pNext = &untyped;
    }
};

class VulkanDevice {
public:
    VulkanDevice(VkInstance                      instance,
                 VkSurfaceKHR                    surface,
                 const std::vector<const char*>& requiredExtensions = {},
                 const std::vector<const char*>& requiredLayers     = {});
    ~VulkanDevice();

    VkPhysicalDevice                        Physical() const { return m_PhysicalDevice; }
    VkDevice                                Logical() const { return m_Device; }
    VkQueue                                 GraphicsQueue() const { return m_GraphicsQueue; }
    VkQueue                                 ComputeQueue() const { return m_ComputeQueue; }
    VkQueue                                 TransferQueue() const { return m_TransferQueue; }
    VkQueue                                 PresentQueue() const { return m_PresentQueue; }
    uint32_t                                GraphicsFamily() const { return m_GraphicsFamily; }
    uint32_t                                ComputeFamily() const { return m_ComputeFamily; }
    uint32_t                                TransferFamily() const { return m_TransferFamily; }
    uint32_t                                PresentFamily() const { return m_PresentFamily; }
    const VkPhysicalDeviceLimits&           Limits() const { return m_PhysicalDeviceProps.properties.limits; }
    const VkPhysicalDeviceProperties&       PhysicalDeviceProperties() const { return m_PhysicalDeviceProps.properties; }
    const VkPhysicalDeviceMemoryProperties& PhysicalDeviceMemoryProperties() const { return m_PhysicalDeviceMemoryProps; }
    const VkPhysicalDeviceDescriptorHeapPropertiesEXT DescriptorHeapProperties() const { return m_DescHeapProps; }

private:
    DeviceInfo GatherDeviceInfo(VkPhysicalDevice device) const;
    void       LogDeviceInfo(uint32_t index, const DeviceInfo& info) const;
    uint32_t   ScoreDevice(const DeviceInfo& info) const;
    int        SelectDevice(const std::vector<DeviceInfo>& devices) const;
    bool       IsDeviceSuitable(VkPhysicalDevice device) const;
    void       RequestFeatures(DeviceFeatureChain& chain) const;
    void       LogEnabledFeatures(const DeviceFeatureChain& chain) const;
    void       LogEnabledExtensions(const std::vector<const char*>& extensions) const;

    bool ValidateExtensions(const std::vector<const char*>& requested, const std::vector<VkExtensionProperties>& available) const;
    void CreateLogicalDevice(const std::vector<const char*>& requiredExtensions, const std::vector<const char*>& requiredLayers);
    void QueryProps();

private:
    VkInstance       m_Instance       = VK_NULL_HANDLE;
    VkSurfaceKHR     m_Surface        = VK_NULL_HANDLE;
    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
    VkDevice         m_Device         = VK_NULL_HANDLE;

    VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
    VkQueue m_ComputeQueue  = VK_NULL_HANDLE;
    VkQueue m_TransferQueue = VK_NULL_HANDLE;
    VkQueue m_PresentQueue  = VK_NULL_HANDLE;

    uint32_t m_GraphicsFamily = UINT32_MAX;
    uint32_t m_ComputeFamily  = UINT32_MAX;
    uint32_t m_TransferFamily = UINT32_MAX;
    uint32_t m_PresentFamily  = UINT32_MAX;

    VkPhysicalDeviceProperties2                 m_PhysicalDeviceProps{};
    VkPhysicalDeviceMemoryProperties            m_PhysicalDeviceMemoryProps{};
    VkPhysicalDeviceDescriptorHeapPropertiesEXT m_DescHeapProps{};
};