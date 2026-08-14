#include "Engine/Gpu/Passes/Gbuffer.h"
#include "Engine/Gpu/Context.h"

namespace Lgt {
namespace Gpu::Pass {

GBuffer::PassContext GBuffer::Context;

const GBuffer::PassContext& GBuffer::GetContext() {
    return Context;
}

void GBuffer::Reset() {
    if (Resources != nullptr) {
        Resources->DestroyTexture(Context.albedo);
        Resources->DestroyTexture(Context.normal);
        Resources->DestroyTexture(Context.depth);
    }
    Context = {};
}

void GBuffer::Execute(FrameGraphPass* pass, void* userdata) {}

void GBuffer::Setup(const FrameGraphBuilder& builder, FrameGraphPass* pass) {

    Context.albedo = Resources->CreateTexture(TextureDesc::ColorAttachment(Renderer->GetCurrentExtent()));
    Context.normal = Resources->CreateTexture(TextureDesc::ColorAttachment(Renderer->GetCurrentExtent()));
    Context.depth  = Resources->CreateTexture(TextureDesc::DepthAttachment(Renderer->GetCurrentExtent()));

    VkImageViewCreateInfo imageViewCi;
    imageViewCi.sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCi.viewType   = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCi.format     = VK_FORMAT_R8G8B8_SRGB;
    imageViewCi.components = {
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY,
    };

    auto texture = Resources->GetTexture(Context.albedo);

    imageViewCi.image                           = texture->image;
    imageViewCi.subresourceRange.baseArrayLayer = 0;
    imageViewCi.subresourceRange.baseMipLevel   = 0;
    imageViewCi.subresourceRange.layerCount     = texture->arrayLayers;
    imageViewCi.subresourceRange.levelCount     = texture->mipLevels;

   // Context.gpuAlbedo = ResourceHeap->AllocateTexture(Context.albedo, imageViewCi, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    pass->name = "GBuffer";
    pass->Writes(GBuffer::Context.albedo); // Albedo
    pass->Writes(GBuffer::Context.normal); // Normal
    pass->Writes(GBuffer::Context.depth);  // Depth
}

} // namespace Gpu::Pass
} // namespace Lgt
