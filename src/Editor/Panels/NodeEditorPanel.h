#pragma once
#include "Editor/Context.h"
#include <imgui_node_editor.h>

namespace Lgt {
namespace Editor::Panel {

class NodeEditorPanel {
public:
    void Init(Context* context);
    void Shutdown();
    void Draw();

private:
    Context* _context = nullptr;
    ax::NodeEditor::EditorContext* editorContext_ = nullptr;
};

} // namespace Editor::Panel
} // namespace Lgt
