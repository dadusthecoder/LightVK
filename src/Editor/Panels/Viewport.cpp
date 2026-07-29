#include "Viewport.h"
#include "Engine/Renderer/Gpu/Context.h"
#include <imgui.h>

namespace Lgt {
namespace Editor::Panel {

void Viewport::Init(Context* context) {
    context_ = context;
}

void Viewport::Shutdown() {
}

void Viewport::Draw() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
    ImGui::Begin("Viewport");

    if (context_) {
        context_->isViewportHovered = ImGui::IsWindowHovered();
        context_->isViewportFocused = ImGui::IsWindowFocused();
    }

    ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
    uint32_t width = static_cast<uint32_t>(viewportPanelSize.x);
    uint32_t height = static_cast<uint32_t>(viewportPanelSize.y);

    if (width > 0 && height > 0) {
        // Resize the offscreen scene target if the ImGui window size changed
        Gpu::g_Renderer->ResizeSceneTarget(width, height);

        VkDescriptorSet textureId = Gpu::g_Renderer->GetSceneTexture();
        if (textureId != VK_NULL_HANDLE) {
            // Draw the offscreen texture filling the available region
            ImGui::Image((ImTextureID)textureId, ImVec2{ (float)width, (float)height });
        }
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

} // namespace Editor::Panel
} // namespace Lgt
