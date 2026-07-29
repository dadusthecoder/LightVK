#include "Context.h"
#include "Engine/Renderer/Vulkan/Context.h"

namespace Lgt::Gpu {

Renderer*       g_Renderer = nullptr;
DescriptorHeap* g_ResourceHeap = nullptr;
DescriptorHeap* g_SamplerHeap = nullptr;

ResourcePool<Texture, TextureHandle> g_Textures;
ResourcePool<Buffer, BufferHandle>   g_Buffers;

void Init(GLFWwindow* window) {
    g_ResourceHeap =
        new DescriptorHeap(MAX_RESOURCES * 32 + Vulkan::g_Device->DescriptorHeapProperties().minResourceHeapReservedRange);
    g_SamplerHeap =
        new DescriptorHeap(MAX_SAMPLERS * 16 + Vulkan::g_Device->DescriptorHeapProperties().minSamplerHeapReservedRange);
    g_Renderer = new Renderer();
    g_Renderer->Init(window);
}

void Shutdown() {
    vkDeviceWaitIdle(Vulkan::g_Device->Logical());

    g_Buffers.ForEach([&](Gpu::Buffer& buffer) {
        if (buffer.mapped)
            Vulkan::g_Allocator->unmap(buffer.allocation);

        Vulkan::g_Allocator->destroyBuffer(buffer.buffer, buffer.allocation);
    });

    g_Buffers.Clear();

    delete g_ResourceHeap;
    delete g_SamplerHeap;

    g_Renderer->ShutDown();
}

} // namespace Lgt::Gpu