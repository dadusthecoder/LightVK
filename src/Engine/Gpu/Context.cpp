
#include "Engine/Gpu/Vulkan/Context.h"
#include "Engine/Gpu/Context.h"
#include "Engine/Gpu/ResourceManager.h"

namespace Lgt::Gpu {

RendererClass*    Renderer     = nullptr;
DescriptorHeap*   ResourceHeap = nullptr;
DescriptorHeap*   SamplerHeap  = nullptr;
ResourceManager*  Resources    = nullptr;
FrameGraphClass* FrameGraph  = nullptr;

void Init(GLFWwindow* window) {

    ResourceHeap = new DescriptorHeap(MAX_RESOURCES * 32, true, false);
    SamplerHeap  = new DescriptorHeap(MAX_SAMPLERS * 16, false, true);

    Resources   = new ResourceManager();
    Renderer    = new RendererClass();
    FrameGraph = new FrameGraphClass();

    Resources->Init();
    Renderer->Init(window);
    FrameGraph->Init();
}

void Shutdown() {

    vkDeviceWaitIdle(Vulkan::g_Device->Logical());

    Resources->Shutdown();
    delete Resources;

    delete ResourceHeap;

    delete SamplerHeap;

    Renderer->ShutDown();
    delete Renderer;

    FrameGraph->ShoutDown();
    delete FrameGraph;
}

} // namespace Lgt::Gpu
