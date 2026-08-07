#include "NodeEditorPanel.h"
#include <imgui.h>

namespace Lgt {
namespace Editor::Panel {

class MainPassNode : public ImFlow::BaseNode {
public:
    explicit MainPassNode() {
        setTitle("Main Pass");
        addIN<int>("In", 0, ImFlow::ConnectionFilter::SameType());
        addOUT<int>("Out")->behaviour([this]() { return 0; });
    }

    void draw() override {
        // Any custom body elements here
    }
};

class PostProcessNode : public ImFlow::BaseNode {
public:
    explicit PostProcessNode() {
        setTitle("Post Process");
        addIN<int>("In", 0, ImFlow::ConnectionFilter::SameType());
        addOUT<int>("Out")->behaviour([this]() { return 0; });
    }

    void draw() override {
        // Any custom body elements here
    }
};

void NodeEditorPanel::Init(Context* context) {
    _context = context;
    _nodeEditor = std::make_unique<ImFlow::ImNodeFlow>();
    
    // Create initial nodes
    _nodeEditor->addNode<MainPassNode>(ImVec2(100, 100));
    _nodeEditor->addNode<PostProcessNode>(ImVec2(400, 100));
}

void NodeEditorPanel::Shutdown() {
    // Destroy ImNodeFlow before ImGui context is destroyed
    _nodeEditor.reset();
}

void NodeEditorPanel::Draw() {
    if (!ImGui::Begin("Render Graph")) {
        ImGui::End();
        return;
    }

    // Process events and draw the node grid
    if (_nodeEditor) {
        _nodeEditor->update();
    }

    ImGui::End();
}

} // namespace Editor::Panel
} // namespace Lgt
