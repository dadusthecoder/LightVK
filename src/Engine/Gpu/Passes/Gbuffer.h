#pragma once
#include "Engine/Core/Core.h"

#include "Engine/Gpu/Renderer.h"
#include "Engine/Gpu/Resource.h"
#include "Engine/Gpu/DescriptorHeap.h"
#include "Engine/Gpu/ResourceManager.h"
#include "Engine/Gpu/FrameGraph.h"

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
    static void Reset();

    static void Setup(const FrameGraphBuilder& builder, FrameGraphPass* pass);
    static void Execute(FrameGraphPass* pass, void* userdata);

private:
    static PassContext Context;
};

} // namespace Gpu::Pass

} // namespace Lgt
