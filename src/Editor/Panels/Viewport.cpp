#include "Viewport.h"
#include "Engine/Gpu/Context.h"
#include "Engine/Scene/Components.h"
#include "Engine/Physics/PhysicsComponents.h"
#include "Engine/Scene/World.h"
#include <imgui.h>
#include <ImGuizmo.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <cmath>

namespace Lgt {
namespace Editor::Panel {

// ── 3D→2D projection helpers removed as wireframe rendering is now done in Vulkan ──

// ── Viewport ──────────────────────────────────────────────────────────────

void Viewport::Init(Context* context) {
    _context = context;
}

void Viewport::Shutdown() {
}

void Viewport::Draw() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
    ImGui::Begin("Viewport");

    if (_context) {
        _context->isViewportHovered = ImGui::IsWindowHovered();
        _context->isViewportFocused = ImGui::IsWindowFocused();
    }

    ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
    uint32_t width = static_cast<uint32_t>(viewportPanelSize.x);
    uint32_t height = static_cast<uint32_t>(viewportPanelSize.y);

    if (width > 0 && height > 0) {
        // Resize the offscreen scene target if the ImGui window size changed
        Gpu::Renderer->ResizeSceneTarget({width, height});

        VkDescriptorSet textureId = Gpu::Renderer->GetSceneTexture();
        if (textureId != VK_NULL_HANDLE) {
            // Draw the offscreen texture filling the available region
            ImGui::Image((ImTextureID)textureId, ImVec2{ (float)width, (float)height });
        }
    }

    // --- ImGuizmo + Collider Visualization ---
    if (_context && _context->world) {
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist();
        
        ImVec2 windowPos = ImGui::GetWindowPos();
        ImVec2 windowMin = ImGui::GetWindowContentRegionMin();
        ImVec2 windowMax = ImGui::GetWindowContentRegionMax();
        
        float viewX = windowPos.x + windowMin.x;
        float viewY = windowPos.y + windowMin.y;
        float viewWidth = windowMax.x - windowMin.x;
        float viewHeight = windowMax.y - windowMin.y;
        
        ImGuizmo::SetRect(viewX, viewY, viewWidth, viewHeight);

        // Find active camera
        glm::mat4 cameraView = glm::mat4(1.0f);
        glm::mat4 cameraProj = glm::mat4(1.0f);
        bool hasCamera = false;

        auto& reg = _context->world->Registry();
        auto cameraViewReg = reg.view<Component::Camera, Component::LocalTransform>();
        for (auto entity : cameraViewReg) {
            auto& cam = cameraViewReg.get<Component::Camera>(entity);
            auto& transform = cameraViewReg.get<Component::LocalTransform>(entity);
            if (cam.isActive) {
                cameraView = cam.ViewMatrix(transform.position);
                cameraProj = cam.ProjectionMatrix(viewWidth / viewHeight);
                cameraProj[1][1] *= -1.0f; // Unflip for ImGuizmo / screen projection
                hasCamera = true;
                break;
            }
        }

        // ── Collider Wireframe Visualization removed (now using Vulkan pipeline) ──
        // ── ImGuizmo Transform Manipulation ──────────────────────────
        Entity selectedEntity = _context->selectedEntity;
        if (hasCamera && selectedEntity.IsValid() && selectedEntity.Has<Component::WorldTransform>()) {
            auto& worldTransform = selectedEntity.Get<Component::WorldTransform>();
            glm::mat4 transformMat = worldTransform.matrix;
            
            ImGuizmo::OPERATION operation = ImGuizmo::TRANSLATE;
            if (_context->currentGizmoOperation == GizmoOperation::Rotate) operation = ImGuizmo::ROTATE;
            if (_context->currentGizmoOperation == GizmoOperation::Scale) operation = ImGuizmo::SCALE;

            ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(cameraProj), 
                                 operation, ImGuizmo::LOCAL, glm::value_ptr(transformMat));

            if (ImGuizmo::IsUsing()) {
                if (selectedEntity.Has<Component::LocalTransform>()) {
                    auto& localTransform = selectedEntity.Get<Component::LocalTransform>();
                    
                    glm::vec3 scale;
                    glm::quat rotation;
                    glm::vec3 translation;
                    glm::vec3 skew;
                    glm::vec4 perspective;
                    
                    glm::decompose(transformMat, scale, rotation, translation, skew, perspective);
                    
                    localTransform.position = translation;
                    localTransform.rotation = rotation;
                    localTransform.scale = scale;
                }
            }
        }
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

} // namespace Editor::Panel
} // namespace Lgt
