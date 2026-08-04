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

    Context* GetContext() { return &_context; }

private:
    Context          _context;
    Panel::Hierarchy _panel_hirearchy;
    Panel::Viewport  _panel_viewport;
    Panel::NodeEditorPanel _panel_node_editor;
    Panel::InspectorPanel  _panel_inspector;
};

} // namespace Editor

} // namespace Lgt
