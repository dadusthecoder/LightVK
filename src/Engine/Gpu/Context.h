#pragma once

#include "Engine/Core/Core.h"

#include "Engine/Gpu/Renderer.h"
#include "Engine/Gpu/Resource.h"
#include "Engine/Gpu/DescriptorHeap.h"
#include "Engine/Gpu/ResourceManager.h"
#include "Engine/Gpu/FrameGraph.h"

namespace Lgt::Gpu {

extern LIGHTVK_API RendererClass*    Renderer;
extern LIGHTVK_API DescriptorHeap*   ResourceHeap;
extern LIGHTVK_API DescriptorHeap*   SamplerHeap;
extern LIGHTVK_API ResourceManager*  Resources;
extern LIGHTVK_API FrameGraphClass* FrameGraph;

void Init(GLFWwindow* window);
void Shutdown();

} // namespace Lgt::Gpu