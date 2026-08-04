#include <iostream>
#include <set>
#include <stdexcept>
#include <string>

#include "Device.h"
#include "Engine/Core/Logger.h"
#include "Engine/Core/VkCheck.h"

static bool HasFlag(VkQueueFlags flags, VkQueueFlags bit) {
    return (flags & bit) == bit;
}

// ─── gatherDeviceInfo ────────────────────────────────────────────────────────

DeviceInfo VulkanDevice::GatherDeviceInfo(VkPhysicalDevice device) const {
    DeviceInfo info;
    info.device = device;

    vkGetPhysicalDeviceProperties(device, &info.properties);
    vkGetPhysicalDeviceFeatures(device, &info.features);
    vkGetPhysicalDeviceMemoryProperties(device, &info.memoryProperties);

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    info.queueFamilies.resize(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, info.queueFamilies.data());

    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    info.extensions.resize(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, info.extensions.data());

    return info;
}

// ─── logDeviceInfo ───────────────────────────────────────────────────────────

void VulkanDevice::LogDeviceInfo(uint32_t index, const DeviceInfo& info) const {
    const auto& props = info.properties;
    const auto& mem   = info.memoryProperties;

    LIGHTVK_INFO("------------------------------------------------------");
    LIGHTVK_INFO("Device [{}]: {}", index, props.deviceName);
    LIGHTVK_INFO("------------------------------------------------------");

    const char* deviceType = "Unknown";
    switch (props.deviceType) {
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
        deviceType = "Discrete GPU";
        break;
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
        deviceType = "Integrated GPU";
        break;
    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
        deviceType = "Virtual GPU";
        break;
    case VK_PHYSICAL_DEVICE_TYPE_CPU:
        deviceType = "CPU";
        break;
    default:
        break;
    }
    LIGHTVK_INFO("  Type: {}", deviceType);

    // Fixed: store formatted string to avoid dangling pointer on default case
    std::string vendorStr;
    const char* vendor = "Unknown";
    switch (props.vendorID) {
    case 0x1002:
        vendor = "AMD";
        break;
    case 0x10DE:
        vendor = "NVIDIA";
        break;
    case 0x8086:
        vendor = "Intel";
        break;
    case 0x13B5:
        vendor = "ARM";
        break;
    default:
       // vendorStr = fmt::format("0x{:04X}", props.vendorID);
       // vendor    = vendorStr.c_str();
        break;
    }
    LIGHTVK_INFO("  Vendor: {}", vendor);

    uint32_t major = VK_API_VERSION_MAJOR(props.apiVersion);
    uint32_t minor = VK_API_VERSION_MINOR(props.apiVersion);
    uint32_t patch = VK_API_VERSION_PATCH(props.apiVersion);
    LIGHTVK_INFO("  API Version: {}.{}.{}", major, minor, patch);
    LIGHTVK_INFO("  Driver Version: {}", props.driverVersion);

    uint64_t totalMemory = 0;
    for (uint32_t i = 0; i < mem.memoryHeapCount; i++) {
        if (mem.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
            totalMemory += mem.memoryHeaps[i].size;
    }
    LIGHTVK_INFO("  VRAM: {} MB", totalMemory / (1024 * 1024));

    LIGHTVK_INFO("  Queue Families:");
    for (uint32_t i = 0; i < info.queueFamilies.size(); i++) {
        const auto& family = info.queueFamilies[i];
        std::string caps;

        if (family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            caps += "Graphics ";
        if (family.queueFlags & VK_QUEUE_COMPUTE_BIT)
            caps += "Compute ";
        if (family.queueFlags & VK_QUEUE_TRANSFER_BIT)
            caps += "Transfer ";
        if (family.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
            caps += "SparseBinding ";

        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(info.device, i, m_Surface, &presentSupport);
        if (presentSupport)
            caps += "Present ";

        LIGHTVK_INFO("    [{}] Count: {}, Flags: {}", i, family.queueCount, caps);
    }

    LIGHTVK_INFO("  Limits:");
    LIGHTVK_INFO("    Max Image Dimension 2D:                    {}", props.limits.maxImageDimension2D);
    LIGHTVK_INFO("    Max Framebuffer Width:                     {}", props.limits.maxFramebufferWidth);
    LIGHTVK_INFO("    Max Framebuffer Height:                    {}", props.limits.maxFramebufferHeight);
    LIGHTVK_INFO("    Max Viewport Dimensions:                   {}x{}",
                 props.limits.maxViewportDimensions[0],
                 props.limits.maxViewportDimensions[1]);
    LIGHTVK_INFO("    Max Bound Descriptor Sets:                 {}", props.limits.maxBoundDescriptorSets);
    LIGHTVK_INFO("    Max Per-Stage Descriptor Samplers:         {}", props.limits.maxPerStageDescriptorSamplers);
    LIGHTVK_INFO("    Max Per-Stage Descriptor Uniform Buffers:  {}", props.limits.maxPerStageDescriptorUniformBuffers);
    LIGHTVK_INFO("    Max Per-Stage Descriptor Storage Buffers:  {}", props.limits.maxPerStageDescriptorStorageBuffers);
    LIGHTVK_INFO("    Max Per-Stage Descriptor Sampled Images:   {}", props.limits.maxPerStageDescriptorSampledImages);
    LIGHTVK_INFO("    Max Compute Shared Memory Size:            {} KB", props.limits.maxComputeSharedMemorySize / 1024);
    LIGHTVK_INFO("    Max Compute Work Group Invocations:        {}", props.limits.maxComputeWorkGroupInvocations);
    LIGHTVK_INFO("    Timestamp Compute and Graphics:            {}", props.limits.timestampComputeAndGraphics ? "Yes" : "No");

    LIGHTVK_INFO("  Score: {}\n", info.score);
}

// ─── scoreDevice ─────────────────────────────────────────────────────────────

uint32_t VulkanDevice::ScoreDevice(const DeviceInfo& info) const {
    uint32_t score = 0;

    if (info.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        score += 10000;
    else if (info.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
        score += 5000;

    score += info.properties.limits.maxImageDimension2D;

    uint64_t vram = 0;
    for (uint32_t i = 0; i < info.memoryProperties.memoryHeapCount; i++) {
        if (info.memoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
            vram += info.memoryProperties.memoryHeaps[i].size;
    }
    score += static_cast<uint32_t>(vram / (1024 * 1024));

    if (info.features.geometryShader)
        score += 100;
    if (info.features.tessellationShader)
        score += 100;
    if (info.features.samplerAnisotropy)
        score += 100;

    bool hasDedicatedCompute  = false;
    bool hasDedicatedTransfer = false;
    for (const auto& family : info.queueFamilies) {
        if (HasFlag(family.queueFlags, VK_QUEUE_COMPUTE_BIT) && !HasFlag(family.queueFlags, VK_QUEUE_GRAPHICS_BIT))
            hasDedicatedCompute = true;

        if (HasFlag(family.queueFlags, VK_QUEUE_TRANSFER_BIT) && !HasFlag(family.queueFlags, VK_QUEUE_GRAPHICS_BIT) &&
            !HasFlag(family.queueFlags, VK_QUEUE_COMPUTE_BIT))
            hasDedicatedTransfer = true;
    }
    if (hasDedicatedCompute)
        score += 500;
    if (hasDedicatedTransfer)
        score += 250;

    return score;
}

// ─── selectDevice ────────────────────────────────────────────────────────────

int VulkanDevice::SelectDevice(const std::vector<DeviceInfo>& devices) const {
#ifndef RX_DEBUG_BUILD
    int      bestIndex = 0;
    uint32_t bestScore = devices[0].score;

    for (size_t i = 1; i < devices.size(); i++) {
        if (devices[i].score > bestScore) {
            bestScore = devices[i].score;
            bestIndex = static_cast<int>(i);
        }
    }

    LIGHTVK_INFO(
        "Auto-selecting device [{}]: {} (highest score: {})", bestIndex, devices[bestIndex].properties.deviceName, bestScore);
    return devices[bestIndex].index;
#else
    LIGHTVK_INFO("Suitable devices:");
    for (const auto& d : devices)
        LIGHTVK_INFO("  [{}] {} - Score: {}", d.index, d.properties.deviceName, d.score);

    int      recommendedInfoIndex = 0;
    uint32_t bestScore            = devices[0].score;

    for (size_t i = 1; i < devices.size(); i++) {
        if (devices[i].score > bestScore) {
            bestScore            = devices[i].score;
            recommendedInfoIndex = static_cast<int>(i);
        }
    }

    LIGHTVK_INFO("Recommended: [{}] {} (highest score: {})",
                 devices[recommendedInfoIndex].index,
                 devices[recommendedInfoIndex].properties.deviceName,
                 bestScore);
    LIGHTVK_INFO("Press Enter to accept, or type a device index to override:");

    std::string input;
    std::getline(std::cin, input);

    if (input.empty())
        return devices[recommendedInfoIndex].index;

    try {
        int selectedIndex = std::stoi(input);
        for (size_t i = 0; i < devices.size(); i++) {
            if (devices[i].index == selectedIndex) {
                LIGHTVK_INFO("Using selected device [{}]: {}", selectedIndex, devices[i].properties.deviceName);
                return selectedIndex;
            }
        }
        LIGHTVK_WARN("Invalid device index {}. Falling back to recommended: {}",
                     selectedIndex,
                     devices[recommendedInfoIndex].properties.deviceName);
    } catch (const std::exception& e) {
        LIGHTVK_WARN("Invalid input '{}' ({}). Falling back to recommended: {}",
                     input,
                     e.what(),
                     devices[recommendedInfoIndex].properties.deviceName);
    }

    return devices[recommendedInfoIndex].index;
#endif
}

// ─── isDeviceSuitable ────────────────────────────────────────────────────────

bool VulkanDevice::IsDeviceSuitable(VkPhysicalDevice device) const {
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
    std::vector<VkQueueFamilyProperties> families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, families.data());

    bool hasGraphics = false;
    bool hasPresent  = false;

    for (uint32_t i = 0; i < count; i++) {
        if (HasFlag(families[i].queueFlags, VK_QUEUE_GRAPHICS_BIT))
            hasGraphics = true;

        VkBool32 present = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Surface, &present);
        if (present)
            hasPresent = true;
    }

    return hasGraphics && hasPresent;
}

// ─── validateExtensions ──────────────────────────────────────────────────────

bool VulkanDevice::ValidateExtensions(const std::vector<const char*>&           requested,
                                      const std::vector<VkExtensionProperties>& available) const {
    bool allSupported = true;

    LIGHTVK_INFO("Validating {} requested extension(s):", requested.size());
    for (const char* reqExt : requested) {
        bool found = false;
        for (const auto& avail : available) {
            if (strcmp(reqExt, avail.extensionName) == 0) {
                found = true;
                break;
            }
        }

        if (found)
            LIGHTVK_INFO("[OK]:{}", reqExt);
        else {
            LIGHTVK_ERROR("[MISSING] {}", reqExt);
            allSupported = false;
        }
    }

    return allSupported;
}

// ─── requestFeatures ─────────────────────────────────────────────────────────

void VulkanDevice::RequestFeatures(DeviceFeatureChain& chain) const {
    // Query device support first so we only enable what exists.
    vkGetPhysicalDeviceFeatures2(m_PhysicalDevice, &chain.core);

    chain.core.features.robustBufferAccess = VK_FALSE;

    // Core 1.0
    chain.core.features.samplerAnisotropy = VK_TRUE;

    // Vulkan 1.2
    chain.vk12.timelineSemaphore                            = VK_TRUE;
    chain.vk12.bufferDeviceAddress                          = VK_TRUE;
    chain.vk12.descriptorIndexing                           = VK_TRUE;
    chain.vk12.runtimeDescriptorArray                       = VK_TRUE;
    chain.vk12.descriptorBindingPartiallyBound              = VK_TRUE;
    chain.vk12.descriptorBindingVariableDescriptorCount     = VK_TRUE;
    chain.vk12.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;

    // Vulkan 1.3
    chain.vk13.dynamicRendering = VK_TRUE;
    chain.vk13.synchronization2 = VK_TRUE;

    // descriptor heap
    chain.descHeap.descriptorHeap = VK_TRUE;

    chain.untyped.shaderUntypedPointers = VK_TRUE;
}

// ─── logEnabledFeatures ──────────────────────────────────────────────────────
void VulkanDevice::LogEnabledFeatures(const DeviceFeatureChain& chain) const {
    const auto& f        = chain.core.features;
    const auto& f12      = chain.vk12;
    const auto& f13      = chain.vk13;
    const auto& descHeap = chain.descHeap;

    LIGHTVK_INFO("Enabled Features:");

    struct Feature {
        const char* label;
        VkBool32    enabled;
        const char* version;
    };

    Feature features[] = {
        // Core 1.0
        {"Sampler Anisotropy", f.samplerAnisotropy, "Core 1.0"},
        {"Geometry Shader", f.geometryShader, "Core 1.0"},
        {"Tessellation Shader", f.tessellationShader, "Core 1.0"},
        {"Multi-Viewport", f.multiViewport, "Core 1.0"},
        {"Fill Mode Non-Solid", f.fillModeNonSolid, "Core 1.0"},
        {"Wide Lines", f.wideLines, "Core 1.0"},
        {"Large Points", f.largePoints, "Core 1.0"},
        {"Texture Compression BC", f.textureCompressionBC, "Core 1.0"},
        {"Texture Compression ETC2", f.textureCompressionETC2, "Core 1.0"},
        {"Texture Compression ASTC", f.textureCompressionASTC_LDR, "Core 1.0"},
        // Vulkan 1.2
        {"Timeline Semaphore", f12.timelineSemaphore, "Vulkan 1.2"},
        {"Buffer Device Address", f12.bufferDeviceAddress, "Vulkan 1.2"},
        {"Descriptor Indexing", f12.descriptorIndexing, "Vulkan 1.2"},
        {"Runtime Descriptor Array", f12.runtimeDescriptorArray, "Vulkan 1.2"},
        {"Descriptor Binding Partially Bound", f12.descriptorBindingPartiallyBound, "Vulkan 1.2"},
        {"Descriptor Binding Variable Descriptor Count", f12.descriptorBindingVariableDescriptorCount, "Vulkan 1.2"},
        {"Descriptor Binding Sampled Image Update After Bind", f12.descriptorBindingSampledImageUpdateAfterBind, "Vulkan 1.2"},
        // Vulkan 1.3
        {"Dynamic Rendering", f13.dynamicRendering, "Vulkan 1.3"},
        {"Synchronization2", f13.synchronization2, "Vulkan 1.3"},

        // descriptor heap
        {"Descriptor Heap", descHeap.descriptorHeap, "DescriptorHeapFeaturesEXT"}

    };

    for (const auto& [label, enabled, version] : features) {
        if (enabled)
            LIGHTVK_INFO("  [{}] {}", version, label);
    }
}

// ─── logEnabledExtensions ────────────────────────────────────────────────────

void VulkanDevice::LogEnabledExtensions(const std::vector<const char*>& extensions) const {
    LIGHTVK_INFO("Enabled Extensions ({}):", extensions.size());
    for (const char* ext : extensions)
        LIGHTVK_INFO("  - {}", ext);
}

// ─── createLogicalDevice ─────────────────────────────────────────────────────

void VulkanDevice::CreateLogicalDevice(const std::vector<const char*>& requiredExtensions,
                                       const std::vector<const char*>& requiredLayers) {
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &count, nullptr);
    std::vector<VkQueueFamilyProperties> families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &count, families.data());

    // ── Find queue families ───────────────────────────────────────────────────
    m_GraphicsFamily = UINT32_MAX;
    m_ComputeFamily  = UINT32_MAX;
    m_TransferFamily = UINT32_MAX;
    m_PresentFamily  = UINT32_MAX;

    for (uint32_t i = 0; i < count; i++) {
        const auto& f = families[i];

        if (HasFlag(f.queueFlags, VK_QUEUE_GRAPHICS_BIT))
            m_GraphicsFamily = i;

        if (HasFlag(f.queueFlags, VK_QUEUE_COMPUTE_BIT) && !HasFlag(f.queueFlags, VK_QUEUE_GRAPHICS_BIT))
            m_ComputeFamily = i;

        if (HasFlag(f.queueFlags, VK_QUEUE_TRANSFER_BIT) && !HasFlag(f.queueFlags, VK_QUEUE_GRAPHICS_BIT) &&
            !HasFlag(f.queueFlags, VK_QUEUE_COMPUTE_BIT))
            m_TransferFamily = i;

        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, i, m_Surface, &presentSupport);
        if (presentSupport)
            m_PresentFamily = i;
    }

    // Fall back to shared families if dedicated ones are unavailable.
    if (m_ComputeFamily == UINT32_MAX)
        m_ComputeFamily = m_GraphicsFamily;
    if (m_TransferFamily == UINT32_MAX)
        m_TransferFamily = m_ComputeFamily;

    LIGHTVK_INFO("Queue Families:");
    LIGHTVK_INFO("  Graphics: {}", m_GraphicsFamily);
    LIGHTVK_INFO("  Compute:  {}", m_ComputeFamily);
    LIGHTVK_INFO("  Transfer: {}", m_TransferFamily);
    LIGHTVK_INFO("  Present:  {}", m_PresentFamily);

    // ── Validate extensions ───────────────────────────────────────────────────
    uint32_t availableCount = 0;
    vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &availableCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(availableCount);
    vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &availableCount, availableExtensions.data());

    std::vector<const char*> actualExtensions = requiredExtensions;
    for (const auto& avail : availableExtensions) {
        if (strcmp("VK_KHR_portability_subset", avail.extensionName) == 0) {
            actualExtensions.push_back("VK_KHR_portability_subset");
            break;
        }
    }

    if (!ValidateExtensions(actualExtensions, availableExtensions))
        throw std::runtime_error("One or more required Vulkan device extensions are not supported.");

    // ── Build queue create infos ──────────────────────────────────────────────
    std::set<uint32_t> uniqueFamilies = {m_GraphicsFamily, m_ComputeFamily, m_TransferFamily};
    float              priority       = 1.0f;

    std::vector<VkDeviceQueueCreateInfo> queues;
    for (uint32_t family : uniqueFamilies) {
        VkDeviceQueueCreateInfo q{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        q.queueFamilyIndex = family;
        q.queueCount       = 1;
        q.pQueuePriorities = &priority;
        queues.push_back(q);
    }

    // ── Request features ──────────────────────────────────────────────────────
    DeviceFeatureChain chain;
    RequestFeatures(chain);

    // ── Create logical device ─────────────────────────────────────────────────
    VkDeviceCreateInfo createInfo{};
    createInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pNext                   = &chain.core;
    createInfo.queueCreateInfoCount    = static_cast<uint32_t>(queues.size());
    createInfo.pQueueCreateInfos       = queues.data();
    createInfo.enabledExtensionCount   = static_cast<uint32_t>(actualExtensions.size());
    createInfo.ppEnabledExtensionNames = actualExtensions.data();
    createInfo.enabledLayerCount       = 0;
    createInfo.ppEnabledLayerNames     = nullptr;

    VK_CHECK(vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device));

    vkGetDeviceQueue(m_Device, m_GraphicsFamily, 0, &m_GraphicsQueue);
    vkGetDeviceQueue(m_Device, m_ComputeFamily, 0, &m_ComputeQueue);
    vkGetDeviceQueue(m_Device, m_TransferFamily, 0, &m_TransferQueue);
    vkGetDeviceQueue(m_Device, m_PresentFamily, 0, &m_PresentQueue);

    // LogEnabledExtensions(requiredExtensions);
    // LogEnabledFeatures(chain);
    QueryProps();
    LIGHTVK_INFO("Logical device created successfully\n");
}

void VulkanDevice::QueryProps() {

    m_PhysicalDeviceProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    m_DescHeapProps.sType       = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_HEAP_PROPERTIES_EXT;
    m_PhysicalDeviceProps.pNext = &m_DescHeapProps;

    vkGetPhysicalDeviceProperties2(m_PhysicalDevice, &m_PhysicalDeviceProps);
    vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &m_PhysicalDeviceMemoryProps);

    LIGHTVK_INFO("");
    LIGHTVK_INFO("=================== DESCRIPTOE_HEAP_PROPS ====================");
    LIGHTVK_INFO("Sampler Descriptor size : {}", m_DescHeapProps.samplerDescriptorSize);
    LIGHTVK_INFO("buffer  Descriptor size : {}", m_DescHeapProps.bufferDescriptorSize);
    LIGHTVK_INFO("image   Descriptor size : {}", m_DescHeapProps.imageDescriptorSize);
    LIGHTVK_INFO("=================== ===================== ====================");
}

// ─── Constructor / Destructor ─────────────────────────────────────────────────
VulkanDevice::VulkanDevice(VkInstance                      instance,
                           VkSurfaceKHR                    surface,
                           const std::vector<const char*>& requiredExtensions,
                           const std::vector<const char*>& requiredLayers)
    : m_Instance(instance),
      m_Surface(surface) {

    uint32_t count = 0;
    vkEnumeratePhysicalDevices(m_Instance, &count, nullptr);

    if (count == 0)
        throw std::runtime_error("No Vulkan-capable physical devices found.");

    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(m_Instance, &count, devices.data());

    std::vector<DeviceInfo> infos;
    for (uint32_t i = 0; i < count; i++) {
        if (!IsDeviceSuitable(devices[i])) {
            LIGHTVK_WARN("Device [{}] skipped (missing graphics or present support)", i);
            continue;
        }

        auto info  = GatherDeviceInfo(devices[i]);
        info.index = static_cast<int>(i);
        info.score = ScoreDevice(info);
        // LogDeviceInfo(i, info);
        infos.push_back(info);
    }

    if (infos.empty())
        throw std::runtime_error("No suitable Vulkan physical device found.");

    m_PhysicalDevice = devices[SelectDevice(infos)];

    CreateLogicalDevice(requiredExtensions, requiredLayers);
}

VulkanDevice::~VulkanDevice() {
    if (m_Device) {
        vkDeviceWaitIdle(m_Device);
        vkDestroyDevice(m_Device, nullptr);
    }
}