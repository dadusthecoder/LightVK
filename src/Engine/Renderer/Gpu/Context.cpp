
#include "Engine/Renderer/Vulkan/Context.h"
#include "Engine/Renderer/Gpu/Context.h"
#include "Engine/Renderer/Gpu/ResourceManager.h"

namespace Lgt::Gpu {

RendererClass*    Renderer     = nullptr;
DescriptorHeap*   ResourceHeap = nullptr;
DescriptorHeap*   SamplerHeap  = nullptr;
ResourceManager*  Resources    = nullptr;
RenderGraphClass* RenderGraph  = nullptr;

void Init(GLFWwindow* window) {
    ResourceHeap =
        new DescriptorHeap(MAX_RESOURCES * 32 + Vulkan::g_Device->DescriptorHeapProperties().minResourceHeapReservedRange);
    SamplerHeap =
        new DescriptorHeap(MAX_SAMPLERS * 16 + Vulkan::g_Device->DescriptorHeapProperties().minSamplerHeapReservedRange);

    Resources = new ResourceManager();
    Resources->Init();

    Renderer = new RendererClass();
    Renderer->Init(window);

    RenderGraph = new RenderGraphClass();
    RenderGraph->Init();
}

void Shutdown() {

    vkDeviceWaitIdle(Vulkan::g_Device->Logical());

    Resources->Shutdown();
    delete Resources;

    delete ResourceHeap;
    
    delete SamplerHeap;

    Renderer->ShutDown();
    delete Renderer;

    RenderGraph->ShoutDown();
    delete RenderGraph;
}

} // namespace Lgt::Gpu