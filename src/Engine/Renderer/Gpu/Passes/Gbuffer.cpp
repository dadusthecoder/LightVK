#include "Engine/Renderer/Gpu/Passes/Gbuffer.h"
#include "Engine/Renderer/Gpu/Context.h"

namespace Lgt {
namespace Gpu::Pass {

GBuffer::PassContext GBuffer::Context;

void GBuffer::Execute(RenderGraphPass* pass, void* userdata) {
    
    

}

void GBuffer::Setup(const RenderGraphBuilder& builder, RenderGraphPass* pass) {

    Context.albedo = Resources->CreateTexture(TextureDesc::ColorAttachment(Renderer->GetSceneExtent()));
    Context.normal = Resources->CreateTexture(TextureDesc::ColorAttachment(Renderer->GetSceneExtent()));
    Context.depth  = Resources->CreateTexture(TextureDesc::DepthAttachment(Renderer->GetSceneExtent()));

    // TODO
    // Context.gpuAlbedo = ResourceHeap->AllocateTexture(albedo);

    pass->name = "GBuffer";
    pass->Writes(GBuffer::Context.albedo); // Albedo
    pass->Writes(GBuffer::Context.normal); // Normal
    pass->Writes(GBuffer::Context.depth);  // Depth
}

} // namespace Gpu::Pass
} // namespace Lgt