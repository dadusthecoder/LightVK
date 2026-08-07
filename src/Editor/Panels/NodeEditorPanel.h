#pragma once
#include <chrono>
#include <cstddef>
#include <string>

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
    static bool   SaveSettings(const char* data, size_t size, ax::NodeEditor::SaveReasonFlags reason, void* userPointer);
    static size_t LoadSettings(char* data, void* userPointer);

    bool   SaveSettingsToDisk(const char* data, size_t size, ax::NodeEditor::SaveReasonFlags reason);
    size_t LoadSettingsFromDisk(char* data);

    Context* _context = nullptr;
    ax::NodeEditor::EditorContext* editorContext_ = nullptr;
    std::string settingsPath_ = "RenderGraph.json";
    std::chrono::steady_clock::time_point lastSettingsWrite_{};
    bool shuttingDown_ = false;
};

} // namespace Editor::Panel
} // namespace Lgt
