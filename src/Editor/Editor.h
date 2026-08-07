#pragma once

#include "Editor/Context.h"
#include "Editor/Panels/Hierarchy.h"
#include "Editor/Panels/Viewport.h"
#include "Editor/Panels/NodeEditorPanel.h"
#include "Editor/Panels/InspectorPanel.h"
#include "Editor/Panels/ConsolePanel.h"
#include "Editor/Panels/ProfilerPanel.h"

#if defined(LIGHTVK_EDITOR_TESTS)
#include "Editor/Tests/EditorTestRunner.h"
#endif

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
    Panel::ConsolePanel    _panel_console;
    Panel::ProfilerPanel   _panel_profiler;
#if defined(LIGHTVK_EDITOR_TESTS)
    Tests::EditorTestRunner _test_runner;
#endif
};

} // namespace Editor

} // namespace Lgt
