#pragma once
#include "Engine/Core/Core.h"

#include "Engine/Renderer/Gpu/Renderer.h"
#include "Engine/Renderer/Gpu/Resource.h"
#include "Engine/Renderer/Gpu/DescriptorHeap.h"
#include "Engine/Renderer/Gpu/ResourceManager.h"
#include "Engine/Renderer/Gpu/RenderGraph.h"

namespace Lgt {
namespace Gpu::Pass {

class GBuffer {
public:
    struct PassContext {

        TextureHandle albedo;
        TextureHandle depth;
        TextureHandle normal;

        uint64_t gpuAlbedo;
        uint64_t gpuNormal;
        uint64_t gpuDepth;
    };

    static const PassContext& GetContext();

    static void Setup(const RenderGraphBuilder& builder, RenderGraphPass* pass);
    static void Execute(RenderGraphPass* pass, void* userdata);

private:
    static PassContext Context;
};

} // namespace Gpu::Pass

} // namespace Lgt
