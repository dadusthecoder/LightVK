#include "Viewport.h"
#include "Engine/Renderer/Gpu/Context.h"
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

// ── 3D→2D projection helpers ──────────────────────────────────────────────

static bool WorldToScreen(const glm::vec3& worldPos, const glm::mat4& viewProj,
                           const ImVec2& viewPos, const ImVec2& viewSize, ImVec2& out) {
    glm::vec4 clip = viewProj * glm::vec4(worldPos, 1.0f);
    if (clip.w <= 0.001f) return false; // behind camera

    glm::vec3 ndc = glm::vec3(clip) / clip.w;
    out.x = viewPos.x + (ndc.x * 0.5f + 0.5f) * viewSize.x;
    out.y = viewPos.y + (-ndc.y * 0.5f + 0.5f) * viewSize.y; // flip Y for screen
    return true;
}

static void DrawLine3D(ImDrawList* dl, const glm::mat4& viewProj,
                        const ImVec2& viewPos, const ImVec2& viewSize,
                        const glm::vec3& a, const glm::vec3& b, ImU32 color) {
    ImVec2 sa, sb;
    if (WorldToScreen(a, viewProj, viewPos, viewSize, sa) &&
        WorldToScreen(b, viewProj, viewPos, viewSize, sb)) {
        dl->AddLine(sa, sb, color, 1.5f);
    }
}

// ── Wireframe drawing ──────────────────────────────────────────────────────

static void DrawWireframeBox(ImDrawList* dl, const glm::mat4& viewProj,
                              const ImVec2& viewPos, const ImVec2& viewSize,
                              const glm::mat4& worldMatrix, const glm::vec3& halfExtents, ImU32 color) {
    // 8 corners of the box in local space, then transformed to world
    glm::vec3 corners[8];
    for (int i = 0; i < 8; i++) {
        corners[i] = glm::vec3(
            (i & 1) ? halfExtents.x : -halfExtents.x,
            (i & 2) ? halfExtents.y : -halfExtents.y,
            (i & 4) ? halfExtents.z : -halfExtents.z
        );
        corners[i] = glm::vec3(worldMatrix * glm::vec4(corners[i], 1.0f));
    }

    // 12 edges of a box
    int edges[12][2] = {
        {0,1}, {2,3}, {4,5}, {6,7}, // X edges
        {0,2}, {1,3}, {4,6}, {5,7}, // Y edges
        {0,4}, {1,5}, {2,6}, {3,7}  // Z edges
    };
    for (auto& e : edges) {
        DrawLine3D(dl, viewProj, viewPos, viewSize, corners[e[0]], corners[e[1]], color);
    }
}

static void DrawWireframeCircle(ImDrawList* dl, const glm::mat4& viewProj,
                                 const ImVec2& viewPos, const ImVec2& viewSize,
                                 const glm::vec3& center, const glm::vec3& axis1, const glm::vec3& axis2,
                                 float radius, ImU32 color, int segments = 32) {
    glm::vec3 prev = center + axis1 * radius;
    for (int i = 1; i <= segments; i++) {
        float angle = (float)i / (float)segments * 6.28318530718f;
        glm::vec3 cur = center + (axis1 * cosf(angle) + axis2 * sinf(angle)) * radius;
        DrawLine3D(dl, viewProj, viewPos, viewSize, prev, cur, color);
        prev = cur;
    }
}

static void DrawWireframeSphere(ImDrawList* dl, const glm::mat4& viewProj,
                                 const ImVec2& viewPos, const ImVec2& viewSize,
                                 const glm::mat4& worldMatrix, float radius, ImU32 color) {
    glm::vec3 center = glm::vec3(worldMatrix[3]);
    // Extract axes from world matrix (ignore scale for the wireframe — use radius directly)
    glm::vec3 right = glm::normalize(glm::vec3(worldMatrix[0]));
    glm::vec3 up    = glm::normalize(glm::vec3(worldMatrix[1]));
    glm::vec3 fwd   = glm::normalize(glm::vec3(worldMatrix[2]));

    DrawWireframeCircle(dl, viewProj, viewPos, viewSize, center, right, up, radius, color);
    DrawWireframeCircle(dl, viewProj, viewPos, viewSize, center, right, fwd, radius, color);
    DrawWireframeCircle(dl, viewProj, viewPos, viewSize, center, up, fwd, radius, color);
}

static void DrawWireframeCapsule(ImDrawList* dl, const glm::mat4& viewProj,
                                  const ImVec2& viewPos, const ImVec2& viewSize,
                                  const glm::mat4& worldMatrix, float halfHeight, float radius, ImU32 color) {
    glm::vec3 center = glm::vec3(worldMatrix[3]);
    glm::vec3 right  = glm::normalize(glm::vec3(worldMatrix[0]));
    glm::vec3 up     = glm::normalize(glm::vec3(worldMatrix[1]));
    glm::vec3 fwd    = glm::normalize(glm::vec3(worldMatrix[2]));

    glm::vec3 top    = center + up * halfHeight;
    glm::vec3 bottom = center - up * halfHeight;

    // Top and bottom circles
    DrawWireframeCircle(dl, viewProj, viewPos, viewSize, top, right, fwd, radius, color);
    DrawWireframeCircle(dl, viewProj, viewPos, viewSize, bottom, right, fwd, radius, color);

    // 4 connecting lines
    DrawLine3D(dl, viewProj, viewPos, viewSize, top + right * radius, bottom + right * radius, color);
    DrawLine3D(dl, viewProj, viewPos, viewSize, top - right * radius, bottom - right * radius, color);
    DrawLine3D(dl, viewProj, viewPos, viewSize, top + fwd * radius, bottom + fwd * radius, color);
    DrawLine3D(dl, viewProj, viewPos, viewSize, top - fwd * radius, bottom - fwd * radius, color);

    // Semicircles on top and bottom for the hemisphere caps
    DrawWireframeCircle(dl, viewProj, viewPos, viewSize, top, right, up, radius, color, 16);
    DrawWireframeCircle(dl, viewProj, viewPos, viewSize, top, fwd, up, radius, color, 16);
    DrawWireframeCircle(dl, viewProj, viewPos, viewSize, bottom, right, up, radius, color, 16);
    DrawWireframeCircle(dl, viewProj, viewPos, viewSize, bottom, fwd, up, radius, color, 16);
}

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

        // ── Collider Wireframe Visualization ──────────────────────────
        if (hasCamera) {
            glm::mat4 viewProj = cameraProj * cameraView;
            ImVec2 viewPos = ImVec2(viewX, viewY);
            ImVec2 viewSize = ImVec2(viewWidth, viewHeight);
            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImU32 wireColor = IM_COL32(50, 220, 50, 180); // green

            // Box colliders
            auto boxView = reg.view<Component::BoxCollider, Component::WorldTransform>();
            for (auto e : boxView) {
                auto& col = boxView.get<Component::BoxCollider>(e);
                auto& wt  = boxView.get<Component::WorldTransform>(e);
                DrawWireframeBox(dl, viewProj, viewPos, viewSize, wt.matrix, col.halfExtents, wireColor);
            }

            // Sphere colliders
            auto sphereView = reg.view<Component::SphereCollider, Component::WorldTransform>();
            for (auto e : sphereView) {
                auto& col = sphereView.get<Component::SphereCollider>(e);
                auto& wt  = sphereView.get<Component::WorldTransform>(e);
                DrawWireframeSphere(dl, viewProj, viewPos, viewSize, wt.matrix, col.radius, wireColor);
            }

            // Capsule colliders
            auto capsuleView = reg.view<Component::CapsuleCollider, Component::WorldTransform>();
            for (auto e : capsuleView) {
                auto& col = capsuleView.get<Component::CapsuleCollider>(e);
                auto& wt  = capsuleView.get<Component::WorldTransform>(e);
                DrawWireframeCapsule(dl, viewProj, viewPos, viewSize, wt.matrix, col.halfHeight, col.radius, wireColor);
            }
        }

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
