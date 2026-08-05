#include "Gbuffer.h"

namespace Lgt {
namespace Gpu::Pass {

void Gbuffer::Execute(RenderGraphPass* pass, void* userdata) {
    LIGHTVK_INFO("Executing {}", pass->name);
}

void Gbuffer::Setup(const RenderGraphBuilder& builder, RenderGraphPass* pass) {
    pass->name = "GBuffer";
    pass->Writes(Gpu::TextureHandle(1)); // Albedo
    pass->Writes(Gpu::TextureHandle(2)); // Normal
    pass->Writes(Gpu::TextureHandle(3)); // Depth
}

} // namespace Gpu::Pass
} // namespace Lgt