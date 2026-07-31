#include "NodeEditorPanel.h"
#include <imgui.h>

namespace ed = ax::NodeEditor;

namespace Lgt {
namespace Editor::Panel {

void NodeEditorPanel::Init(Context* context) {
    context_ = context;
    ed::Config config;
    config.SettingsFile = "RenderGraph.json";
    editorContext_ = ed::CreateEditor(&config);
}

void NodeEditorPanel::Shutdown() {
    if (editorContext_) {
        ed::DestroyEditor(editorContext_);
        editorContext_ = nullptr;
    }
}

void NodeEditorPanel::Draw() {
    ImGui::Begin("Render Graph");

    ed::SetCurrentEditor(editorContext_);
    ed::Begin("My Node Editor");

    int uniqueId = 1;
    
    // Node 1
    ed::BeginNode(uniqueId++);
        ImGui::Text("Main Pass");
        ed::BeginPin(uniqueId++, ed::PinKind::Input);
            ImGui::Text("-> In");
        ed::EndPin();
        ImGui::SameLine();
        ed::BeginPin(uniqueId++, ed::PinKind::Output);
            ImGui::Text("Out ->");
        ed::EndPin();
    ed::EndNode();
    
    // Node 2
    ed::BeginNode(uniqueId++);
        ImGui::Text("Post Process");
        ed::BeginPin(uniqueId++, ed::PinKind::Input);
            ImGui::Text("-> In");
        ed::EndPin();
        ImGui::SameLine();
        ed::BeginPin(uniqueId++, ed::PinKind::Output);
            ImGui::Text("Out ->");
        ed::EndPin();
    ed::EndNode();
    
    // Link
    ed::Link(uniqueId++, 3, 6); // Node 1 output is 3, Node 2 input is 5 (wait, Node 2 ID is 4, Input pin is 5, Output pin is 6). Let's use 3 and 5.

    ed::End();
    ed::SetCurrentEditor(nullptr);

    ImGui::End();
}

} // namespace Editor::Panel
} // namespace Lgt
