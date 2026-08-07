#include "NodeEditorPanel.h"
#include <imgui.h>

#include <fstream>

namespace ed = ax::NodeEditor;

namespace Lgt {
namespace Editor::Panel {

void NodeEditorPanel::Init(Context* context) {
    _context = context;
    ed::Config config;
    config.SettingsFile = nullptr;
    config.SaveSettings = &NodeEditorPanel::SaveSettings;
    config.LoadSettings = &NodeEditorPanel::LoadSettings;
    config.UserPointer   = this;
    editorContext_ = ed::CreateEditor(&config);
}

void NodeEditorPanel::Shutdown() {
    if (editorContext_) {
        shuttingDown_ = true;
        ed::DestroyEditor(editorContext_);
        editorContext_ = nullptr;
        shuttingDown_ = false;
    }
}

bool NodeEditorPanel::SaveSettings(const char* data,
                                   size_t size,
                                   ed::SaveReasonFlags reason,
                                   void* userPointer) {
    return static_cast<NodeEditorPanel*>(userPointer)->SaveSettingsToDisk(data, size, reason);
}

size_t NodeEditorPanel::LoadSettings(char* data, void* userPointer) {
    return static_cast<NodeEditorPanel*>(userPointer)->LoadSettingsFromDisk(data);
}

bool NodeEditorPanel::SaveSettingsToDisk(const char* data, size_t size, ed::SaveReasonFlags reason) {
    const bool navigationOnly = (reason & ed::SaveReasonFlags::Navigation) == ed::SaveReasonFlags::Navigation &&
                                 (static_cast<uint32_t>(reason) & ~static_cast<uint32_t>(ed::SaveReasonFlags::Navigation)) == 0;

    const auto now = std::chrono::steady_clock::now();
    if (navigationOnly && !shuttingDown_ && now - lastSettingsWrite_ < std::chrono::milliseconds(200))
        return false;

    std::ofstream settingsFile(settingsPath_, std::ios::binary | std::ios::trunc);
    if (!settingsFile)
        return false;

    settingsFile.write(data, static_cast<std::streamsize>(size));
    if (!settingsFile)
        return false;

    lastSettingsWrite_ = now;
    return true;
}

size_t NodeEditorPanel::LoadSettingsFromDisk(char* data) {
    std::ifstream settingsFile(settingsPath_, std::ios::binary);
    if (!settingsFile)
        return 0;

    settingsFile.seekg(0, std::ios::end);
    const auto size = static_cast<size_t>(settingsFile.tellg());
    settingsFile.seekg(0, std::ios::beg);

    if (data != nullptr)
        settingsFile.read(data, static_cast<std::streamsize>(size));

    return size;
}

void NodeEditorPanel::Draw() {
    if (!ImGui::Begin("Render Graph")) {
        ImGui::End();
        return;
    }

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

    // Link the first node's output pin to the second node's input pin.
    ed::Link(uniqueId++, 3, 5);

    ed::End();
    ed::SetCurrentEditor(nullptr);

    ImGui::End();
}

} // namespace Editor::Panel
} // namespace Lgt
