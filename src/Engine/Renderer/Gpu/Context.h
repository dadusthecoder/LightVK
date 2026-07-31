#pragma once

#include "Engine/Core/Core.h"

#include "Engine/Renderer/Gpu/Renderer.h"
#include "Engine/Renderer/Gpu/Resource.h"
#include "Engine/Renderer/Gpu/DescriptorHeap.h"
#include "Engine/Renderer/Gpu/ResourceManager.h"
#include "Engine/Renderer/Gpu/RenderGraph.h"

namespace Lgt::Gpu {

extern LIGHTVK_API RendererClass*    Renderer;
extern LIGHTVK_API DescriptorHeap*   ResourceHeap;
extern LIGHTVK_API DescriptorHeap*   SamplerHeap;
extern LIGHTVK_API ResourceManager*  Resources;
extern LIGHTVK_API RenderGraphClass* RenderGraph;

void Init(GLFWwindow* window);
void Shutdown();

} // namespace Lgt::Gpu