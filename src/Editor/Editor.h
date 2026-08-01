#pragma once

#include "Editor/Context.h"
#include "Editor/Panels/Hierarchy.h"
#include "Editor/Panels/Viewport.h"
#include "Editor/Panels/NodeEditorPanel.h"
#include "Editor/Panels/InspectorPanel.h"

struct GLFWwindow;

namespace Lgt {

namespace Editor {
class Editor {
public:
    void Init(World* world);
    void Update();
    void Shutdown();

    Context* GetContext() { return &context_; }

private:
    Context          context_;
    Panel::Hierarchy panel_hirearchy_;
    Panel::Viewport  panel_viewport_;
    Panel::NodeEditorPanel panel_node_editor_;
    Panel::InspectorPanel  panel_inspector_;
};

} // namespace Editor

} // namespace Lgt
