#pragma once

#include "Engine/Core/Core.h"
#include "Renderer.h"
#include "Resource.h"
#include "DescriptorHeap.h"

namespace Lgt::Gpu {

extern LIGHTVK_API Renderer*       g_Renderer;
extern LIGHTVK_API DescriptorHeap* g_ResourceHeap;
extern LIGHTVK_API DescriptorHeap* g_SamplerHeap;

void Init(GLFWwindow* window);  
void Shutdown();
extern LIGHTVK_API ResourcePool<Texture, TextureHandle> g_Textures;
extern LIGHTVK_API ResourcePool<Buffer, BufferHandle>   g_Buffers;

} // namespace Lgt::Gpu