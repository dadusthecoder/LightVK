#include "SceneGraph.h"
#include "Engine/Scene/Entity.h"
#include "Engine/Scene/Components.h"
#include "Engine/Core/Logger.h"

namespace Lgt {

void SceneGraph::Detach(Entity entity) {

    auto& entity_h = entity.Get<Component::Hierarchy>();

    if (!entity_h.parent)
        return;

    auto& parent_h = entity_h.parent.Get<Component::Hierarchy>();

    if (entity_h.nextSibling) {
        entity_h.nextSibling.Get<Component::Hierarchy>().prevSibling = entity_h.prevSibling;
    }

    if (entity_h.prevSibling) {
        entity_h.prevSibling.Get<Component::Hierarchy>().nextSibling = entity_h.nextSibling;
    } else {
        parent_h.firstChild = entity_h.nextSibling;
    }

    entity_h.parent      = Entity::Null();
    entity_h.nextSibling = Entity::Null();
    entity_h.prevSibling = Entity::Null();
}

void SceneGraph::Attach(Entity child, Entity parent) {

    // detach the child from the current parent before attaching to the new parent;
    Detach(child);

    auto& parent_h = parent.Get<Component::Hierarchy>();
    auto& child_h  = child.Get<Component::Hierarchy>();

    if (parent_h.firstChild) {
        auto& parent_child_h       = parent_h.firstChild.Get<Component::Hierarchy>();
        parent_child_h.prevSibling = child;
    }

    child_h.nextSibling = parent_h.firstChild;
    child_h.prevSibling = Entity::Null();
    child_h.parent      = parent;
    parent_h.firstChild = child;
}

// this function handels the graph validation
void SceneGraph::SetParent(Entity parent, Entity child) {

    if (child == parent) {
        LIGHTVK_WARN("Cannot parent an entity to itself.");
        return;
    }

    // check if the new parent is decscedant of the child or not
    if (IsDescendant(parent, child)) {
        LIGHTVK_WARN("Cannot set parent: the specified parent is a descendant of the child. This would create a cycle in the "
                     "scene hierarchy.");
        return;
    }

    Attach(child, parent);
}

void SceneGraph::Reparent(Entity child, Entity newParent) {
    if (child == newParent) {
        LIGHTVK_WARN("Cannot parent an entity to itself.");
        return;
    }
    if (IsDescendant(newParent, child)) {
        LIGHTVK_WARN("Cannot reparent: the specified new parent is a descendant of the child.");
        return;
    }
    Attach(child, newParent);
}

void SceneGraph::Unparent(Entity child) {
    Detach(child);
}

bool SceneGraph::IsDescendant(Entity entity, Entity parent) const noexcept {
    auto current = entity;
    while (current) {
        if (parent == current)
            return true;
        current = current.Get<Component::Hierarchy>().parent;
    }

    return false;
}

} // namespace  Lgt
