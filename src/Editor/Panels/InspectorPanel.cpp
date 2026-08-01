#include "InspectorPanel.h"
#include "Editor/InspectorRegistry.h"
#include "Editor/Widgets.h"
#include "Engine/Scene/Components.h"
#include "imgui.h"

namespace Lgt::Editor::Panel {

void InspectorPanel::Init(Context* context) {
    context_ = context;
}

void InspectorPanel::Shutdown() {
}

void InspectorPanel::Draw() {
    ImGui::Begin("Inspector");

    Entity entity = context_->selectedEntity;
    
    if (!entity.IsValid()) {
        ImGui::TextDisabled("No Entity Selected");
        ImGui::End();
        return;
    }

    DrawEntityHeader(entity);
    ImGui::Spacing();
    
    DrawLiveStatusBar(entity);
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    DrawComponentTimeline(entity);
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    DrawComponents(entity);
    
    ImGui::Spacing();
    DrawAddComponentMenu(entity);

    ImGui::End();
}

void InspectorPanel::DrawEntityHeader(Entity entity) {
    if (entity.Has<Component::Tag>()) {
        auto& tag = entity.Get<Component::Tag>();
        char buffer[256];
        memset(buffer, 0, sizeof(buffer));
        strncpy(buffer, tag.name.c_str(), sizeof(buffer) - 1);
        
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputText("##Name", buffer, sizeof(buffer))) {
            tag.name = std::string(buffer);
        }
    } else {
        ImGui::Text("Entity %d", (uint32_t)entity.Handle());
    }
}

void InspectorPanel::DrawLiveStatusBar(Entity entity) {
    ImGui::BeginGroup();
    Widgets::LiveBadge("GPU Resident", true, IM_COL32(50, 200, 50, 255));
    ImGui::SameLine();
    Widgets::LiveBadge("Visible", true, IM_COL32(50, 150, 255, 255));
    ImGui::SameLine();
    Widgets::LiveBadge("Selected", true, IM_COL32(200, 150, 50, 255));
    ImGui::EndGroup();
}

void InspectorPanel::DrawComponentTimeline(Entity entity) {
    ImGui::TextDisabled("Component Timeline");
    // Draw a fake timeline for now to fulfill the UI design requirements
    ImGui::Text("Transform -> Rendering");
}

void InspectorPanel::DrawComponents(Entity entity) {
    const auto& inspectors = InspectorRegistry::GetInspectors();
    
    for (const auto& [type_hash, inspectorData] : inspectors) {
        // EnTT doesn't give us a direct way to iterate components on an entity by type hash dynamically without a reflection system.
        // We will do a manual check for known types just for this implementation until full reflection is added.
    }
    
    // Manual dispatch for now to avoid building a complex dynamic Any type router
    bool isExpanded = true, isEnabled = true;
    
    if (entity.Has<Component::LocalTransform>()) {
        if (Widgets::DrawComponentCardBegin("LocalTransform", isExpanded, isEnabled, false)) {
            inspectors.at(entt::type_id<Component::LocalTransform>().hash()).drawCallback(entity, &entity.Get<Component::LocalTransform>());
        }
        Widgets::DrawComponentCardEnd();
    }
    
    if (entity.Has<Component::Camera>()) {
        if (Widgets::DrawComponentCardBegin("Camera", isExpanded, isEnabled)) {
            inspectors.at(entt::type_id<Component::Camera>().hash()).drawCallback(entity, &entity.Get<Component::Camera>());
        }
        Widgets::DrawComponentCardEnd();
    }

    if (entity.Has<Component::Material>()) {
        if (Widgets::DrawComponentCardBegin("Material", isExpanded, isEnabled)) {
            inspectors.at(entt::type_id<Component::Material>().hash()).drawCallback(entity, &entity.Get<Component::Material>());
        }
        Widgets::DrawComponentCardEnd();
    }
    
    if (entity.Has<Component::Mesh>()) {
        if (Widgets::DrawComponentCardBegin("Mesh", isExpanded, isEnabled)) {
            inspectors.at(entt::type_id<Component::Mesh>().hash()).drawCallback(entity, &entity.Get<Component::Mesh>());
        }
        Widgets::DrawComponentCardEnd();
    }
    
    if (entity.Has<Component::DirectionalLight>()) {
        if (Widgets::DrawComponentCardBegin("Directional Light", isExpanded, isEnabled)) {
            inspectors.at(entt::type_id<Component::DirectionalLight>().hash()).drawCallback(entity, &entity.Get<Component::DirectionalLight>());
        }
        Widgets::DrawComponentCardEnd();
    }
    
    if (entity.Has<Component::PointLight>()) {
        if (Widgets::DrawComponentCardBegin("Point Light", isExpanded, isEnabled)) {
            inspectors.at(entt::type_id<Component::PointLight>().hash()).drawCallback(entity, &entity.Get<Component::PointLight>());
        }
        Widgets::DrawComponentCardEnd();
    }
}

void InspectorPanel::DrawAddComponentMenu(Entity entity) {
    if (ImGui::Button("Add Component", ImVec2(-1, 30))) {
        ImGui::OpenPopup("AddComponentPopup");
    }

    if (ImGui::BeginPopup("AddComponentPopup")) {
        const auto& inspectors = InspectorRegistry::GetInspectors();
        
        // Manual checks for now
        if (!entity.Has<Component::Camera>() && ImGui::MenuItem("Camera")) entity.Add<Component::Camera>();
        if (!entity.Has<Component::Material>() && ImGui::MenuItem("Material")) entity.Add<Component::Material>();
        if (!entity.Has<Component::Mesh>() && ImGui::MenuItem("Mesh")) entity.Add<Component::Mesh>();
        if (!entity.Has<Component::DirectionalLight>() && ImGui::MenuItem("Directional Light")) entity.Add<Component::DirectionalLight>();
        if (!entity.Has<Component::PointLight>() && ImGui::MenuItem("Point Light")) entity.Add<Component::PointLight>();

        ImGui::EndPopup();
    }
}

} // namespace Lgt::Editor::Panel

// -----------------------------------------------------------------------------
// Component Inspector Registrations
// -----------------------------------------------------------------------------
using namespace Lgt::Editor;

void DrawTransformInspector(Lgt::Entity entity, void* data) {
    auto& transform = *static_cast<Lgt::Component::LocalTransform*>(data);
    Widgets::DrawVec3Control("Position", transform.position);
    
    glm::vec3 euler = glm::degrees(glm::eulerAngles(transform.rotation));
    if (Widgets::DrawVec3Control("Rotation", euler)) {
        transform.rotation = glm::quat(glm::radians(euler));
    }
    
    Widgets::DrawVec3Control("Scale", transform.scale, 1.0f);
}
REGISTER_INSPECTOR(Lgt::Component::LocalTransform, DrawTransformInspector);

void DrawCameraInspector(Lgt::Entity entity, void* data) {
    auto& camera = *static_cast<Lgt::Component::Camera*>(data);
    ImGui::Checkbox("Active", &camera.isActive);
    ImGui::SliderFloat("FOV", &camera.fov, 10.0f, 150.0f);
    ImGui::DragFloat("Near Plane", &camera.nearPlane, 0.1f);
    ImGui::DragFloat("Far Plane", &camera.farPlane, 10.0f);
}
REGISTER_INSPECTOR(Lgt::Component::Camera, DrawCameraInspector);

void DrawMaterialInspector(Lgt::Entity entity, void* data) {
    auto& material = *static_cast<Lgt::Component::Material*>(data);
    Widgets::DrawColorControl("Albedo", material.albedo);
    ImGui::SliderFloat("Roughness", &material.roughness, 0.0f, 1.0f);
    ImGui::SliderFloat("Metallic", &material.metallic, 0.0f, 1.0f);
    ImGui::DragFloat("Emissive", &material.emissiveStrength, 0.1f, 0.0f, 100.0f);
}
REGISTER_INSPECTOR(Lgt::Component::Material, DrawMaterialInspector);

void DrawDirectionalLightInspector(Lgt::Entity entity, void* data) {
    auto& light = *static_cast<Lgt::Component::DirectionalLight*>(data);
    Widgets::DrawVec3Control("Direction", light.direction);
    ImGui::ColorEdit3("Color", glm::value_ptr(light.color));
    ImGui::DragFloat("Intensity", &light.intensity, 0.1f, 0.0f, 100.0f);
}
REGISTER_INSPECTOR(Lgt::Component::DirectionalLight, DrawDirectionalLightInspector);

void DrawPointLightInspector(Lgt::Entity entity, void* data) {
    auto& light = *static_cast<Lgt::Component::PointLight*>(data);
    ImGui::ColorEdit3("Color", glm::value_ptr(light.color));
    ImGui::DragFloat("Intensity", &light.intensity, 0.1f, 0.0f, 100.0f);
    ImGui::DragFloat("Radius", &light.radius, 0.1f, 0.0f, 1000.0f);
}
REGISTER_INSPECTOR(Lgt::Component::PointLight, DrawPointLightInspector);

void DrawMeshInspector(Lgt::Entity entity, void* data) {
    auto& mesh = *static_cast<Lgt::Component::Mesh*>(data);
    ImGui::Text("Vertex Buffer Index: %d", mesh.vertexBufferIndex);
    ImGui::Text("Index Buffer Index: %d", mesh.indexBufferIndex);
    ImGui::Text("Material Index: %d", mesh.materialIndex);
}
REGISTER_INSPECTOR(Lgt::Component::Mesh, DrawMeshInspector);
