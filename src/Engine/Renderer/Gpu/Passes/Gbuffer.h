#pragma once
#include "Engine/Core/Core.h"

#include "Engine/Renderer/Gpu/Renderer.h"
#include "Engine/Renderer/Gpu/Resource.h"
#include "Engine/Renderer/Gpu/DescriptorHeap.h"
#include "Engine/Renderer/Gpu/ResourceManager.h"
#include "Engine/Renderer/Gpu/RenderGraph.h"

namespace Lgt {
namespace Gpu::Pass {

class Gbuffer {
public:
    static void Execute(RenderGraphPass* pass, void* userdata);
    static void Setup(const RenderGraphBuilder& builder, RenderGraphPass* pass);
};
} // namespace Gpu::Pass

} // namespace Lgt
