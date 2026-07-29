#include "Context.h"
#include "Engine/Core/Logger.h"
#include "Engine/Core/VkCheck.h"

namespace Lgt::Vulkan {
VulkanSurface*                 g_Surface = nullptr;
VulkanInstance*                g_Instance = nullptr;
VulkanDevice*                  g_Device = nullptr;
VulkanAllocator*               g_Allocator = nullptr;
VulkanLoadTimeStagingUploader* g_Uploader = nullptr;

void Init(GLFWwindow* window) {
    LGT_ASSERT(volkInitialize() == VK_SUCCESS, "Filed to load the vulkan-1.dll");

    g_Instance = new VulkanInstance();
    g_Surface  = new VulkanSurface();

    // create Vulkan Instance and load it using volk before creating the device
    g_Instance->Init(ENABLE_VALIDATION, VALIDATION_LAYERS);
    volkLoadInstance(g_Instance->Handle());

    // create surface
    g_Surface->Init(g_Instance->Handle(), window);

    // create vulkan device after loding instance with volk
    g_Device = new VulkanDevice(g_Instance->Handle(), g_Surface->Handle(), DEVICE_EXTENSIONS, VALIDATION_LAYERS);
    volkLoadDevice(g_Device->Logical());

    g_Allocator = new VulkanAllocator(g_Instance->Handle(), g_Device->Physical(), g_Device->Logical());

    g_Uploader = new VulkanLoadTimeStagingUploader();
}

void Shutdown() {
    vkDeviceWaitIdle(g_Device->Logical());

    delete g_Uploader;
    delete g_Allocator;
    delete g_Device;

    g_Surface->ShutDown();
    g_Instance->ShutDown();
}

} // namespace Lgt::Vulkan
