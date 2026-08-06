#include "Hierarchy.h"
#include "Engine/Core/Logger.h"
#include "Engine/Scene/World.h"
#include "Engine/Scene/Entity.h"
#include "Engine/Scene/Components.h"

#include "imgui.h"

namespace Lgt {
namespace Editor {

void Panel::Hierarchy::Init(Context* context) {
    _context = context;
}

void Panel::Hierarchy::Shutdown() {}

void Panel::Hierarchy::Draw() {
    ImGui::Begin("Hierarchy");
    auto view = _context->world->Registry().view<Component::Hierarchy>();
    for (auto handle : view) {
        auto entity   = Entity(handle, _context->world);
        auto entity_h = entity.Get<Component::Hierarchy>();

        if (!entity_h.parent)
            DrawNode(entity);
    }

    if (ImGui::BeginPopupContextWindow("HierarchyContextMenu", ImGuiPopupFlags_NoOpenOverItems)) {
        if (ImGui::MenuItem("Create Entity")) {
            _context->world->CreateEntity("New Entity");
        }
        ImGui::EndPopup();
    }

    ImGui::End();
}

void Panel::Hierarchy::DrawNode(Entity entity) {

    auto& entity_h = entity.Get<Component::Hierarchy>();
    auto  child    = entity_h.firstChild;

    // Configure flags based on state
    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;

    if (!entity_h.firstChild) {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }

    if (_context->selectedEntity == entity)
        flags |= ImGuiTreeNodeFlags_Selected;

    ImGui::PushID(static_cast<int>(entt::to_integral(entity.Handle())));

    bool is_open = ImGui::TreeNodeEx("##Node", flags, "%s", entity.Get<Component::Tag>().name.c_str());

    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
        _context->selectedEntity = entity;

    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
        entt::entity handle = entity.Handle();
        ImGui::SetDragDropPayload("HIERARCHY_ENTITY", &handle, sizeof(entt::entity));
        ImGui::Text("%s", entity.Get<Component::Tag>().name.c_str());
        ImGui::EndDragDropSource();
    }

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_ENTITY")) {
            entt::entity droppedHandle = *(entt::entity*)payload->Data;
            Entity droppedEntity(droppedHandle, _context->world);
            if (droppedEntity != entity) {
                _context->world->Graph().Reparent(droppedEntity, entity);
            }
        }
        ImGui::EndDragDropTarget();
    }

    if (ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("Create Child")) {
            auto child = _context->world->CreateEntity("New Entity");
            _context->world->Graph().Reparent(child, entity);
        }
        if (ImGui::MenuItem("Delete")) {
            _context->world->DestroyEntity(entity);
            if (_context->selectedEntity == entity)
                _context->selectedEntity = Entity::Null();
        }
        if (ImGui::MenuItem("Unparent") && entity.Get<Component::Hierarchy>().parent.IsValid()) {
            _context->world->Graph().Unparent(entity);
        }
        ImGui::EndPopup();
    }

    if (is_open && child) {
        while (child) {
            DrawNode(child);
            child = child.Get<Component::Hierarchy>().nextSibling;
        }
        ImGui::TreePop();
    }

    ImGui::PopID();
}

} // namespace Editor
} // namespace Lgt