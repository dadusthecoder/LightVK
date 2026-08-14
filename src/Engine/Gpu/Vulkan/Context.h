#pragma once
#include "Engine/Core/Core.h"
#include "Device.h"
#include "Instance.h"
#include "Allocator.h"
#include "Surface.h"
#include "Swapchain.h"
#include "Uploader.h"

namespace Lgt::Vulkan {

extern LIGHTVK_API VulkanSurface*                 g_Surface;
extern LIGHTVK_API VulkanInstance*                g_Instance;
extern LIGHTVK_API VulkanDevice*                  g_Device;
extern LIGHTVK_API VulkanAllocator*               g_Allocator;
extern LIGHTVK_API VulkanLoadTimeStagingUploader* g_Uploader;

void Init(GLFWwindow* window);
void Shutdown();

} // namespace Lgt::Vulkan
