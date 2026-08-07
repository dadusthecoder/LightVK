
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

    ResourceHeap = new DescriptorHeap(MAX_RESOURCES * 32, true, false);
    SamplerHeap  = new DescriptorHeap(MAX_SAMPLERS * 16, false, true);

    Resources   = new ResourceManager();
    Renderer    = new RendererClass();
    RenderGraph = new RenderGraphClass();

    Resources->Init();
    Renderer->Init(window);
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
