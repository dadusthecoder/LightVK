#pragma once
#include "Engine/Scene/Entity.h"
#include "Engine/Scene/Components.h"
#include "Engine/Physics/PhysicsComponents.h"
#include <unordered_map>
#include <functional>

namespace Lgt {

namespace Editor {
enum class GizmoOperation {
    Translate,
    Rotate,
    Scale
};

enum class EditorMode {
    Edit,
    Play
};

struct Context {
    World* world          = nullptr;
    Entity selectedEntity = Entity::Null();

    bool isViewportHovered = false;
    bool isViewportFocused = false;

    GizmoOperation currentGizmoOperation = GizmoOperation::Translate;
    EditorMode     mode                  = EditorMode::Edit;

    // Snapshot of all LocalTransforms before entering Play mode
    std::unordered_map<uint32_t, Component::LocalTransform> transformSnapshot;

    // Callbacks set by the application to trigger play/stop
    std::function<void()> onPlay;
    std::function<void()> onStop;

    // Callbacks for file operations
    std::string currentScenePath;
    std::function<void()> onSaveScene;
    std::function<void()> onSaveSceneAs;
    std::function<void()> onLoadScene;
};

} // namespace Editor

} // namespace Lgt