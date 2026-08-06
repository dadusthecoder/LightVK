#include "ConsolePanel.h"
#include "Engine/Core/LogBuffer.h"
#include <imgui.h>
#include <cstring>

namespace Lgt::Editor::Panel {

void ConsolePanel::Init(Context* context) {
    _context = context;
}

void ConsolePanel::Shutdown() {
}

void ConsolePanel::Draw() {
    if (!ImGui::Begin("Console")) {
        ImGui::End();
        return;
    }

    // Top toolbar
    if (ImGui::Button("Clear")) {
        LogBuffer::Clear();
    }
    ImGui::SameLine();
    
    ImGui::PushStyleColor(ImGuiCol_Button, _showTrace ? ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive) : ImGui::GetStyleColorVec4(ImGuiCol_Button));
    if (ImGui::Button("Trace")) _showTrace = !_showTrace;
    ImGui::PopStyleColor();
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, _showInfo ? ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive) : ImGui::GetStyleColorVec4(ImGuiCol_Button));
    if (ImGui::Button("Info")) _showInfo = !_showInfo;
    ImGui::PopStyleColor();
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, _showWarn ? ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive) : ImGui::GetStyleColorVec4(ImGuiCol_Button));
    if (ImGui::Button("Warn")) _showWarn = !_showWarn;
    ImGui::PopStyleColor();
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, _showError ? ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive) : ImGui::GetStyleColorVec4(ImGuiCol_Button));
    if (ImGui::Button("Error")) _showError = !_showError;
    ImGui::PopStyleColor();
    ImGui::SameLine();

    ImGui::Checkbox("Auto-scroll", &_autoScroll);
    ImGui::SameLine();

    ImGui::Text("Filter:");
    ImGui::SameLine();
    ImGui::InputText("##Filter", _filterText, IM_ARRAYSIZE(_filterText));

    ImGui::Separator();

    // Main area
    const float footerHeightToReserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footerHeightToReserve), false, ImGuiWindowFlags_HorizontalScrollbar)) {
        
        if (ImGui::BeginTable("ConsoleTable", 3, ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody)) {
            ImGui::TableSetupColumn("Level", ImGuiTableColumnFlags_WidthFixed, 40.0f);
            ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("Message", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            LogBuffer::ForEach([&](const LogEntry& entry) {
                // Apply level filter
                if (entry.level == spdlog::level::trace && !_showTrace) return;
                if (entry.level == spdlog::level::debug && !_showTrace) return; // Map debug to trace for filter
                if (entry.level == spdlog::level::info && !_showInfo) return;
                if (entry.level == spdlog::level::warn && !_showWarn) return;
                if (entry.level >= spdlog::level::err && !_showError) return;

                // Apply text filter
                if (_filterText[0] != '\0') {
                    if (strstr(entry.message.c_str(), _filterText) == nullptr) return;
                }

                ImGui::TableNextRow();
                
                ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // Default Info
                const char* levelStr = "I";
                
                if (entry.level == spdlog::level::trace || entry.level == spdlog::level::debug) {
                    color = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
                    levelStr = "T";
                } else if (entry.level == spdlog::level::warn) {
                    color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
                    levelStr = "W";
                } else if (entry.level == spdlog::level::err) {
                    color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
                    levelStr = "E";
                } else if (entry.level == spdlog::level::critical) {
                    color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
                    levelStr = "C";
                }

                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(color, "[%s]", levelStr);
                
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(color, "%s", entry.timestamp.c_str());
                
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(color, "%s", entry.message.c_str());
            });

            ImGui::EndTable();
        }

        if (_autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();

    ImGui::End();
}

} // namespace Lgt::Editor::Panel
