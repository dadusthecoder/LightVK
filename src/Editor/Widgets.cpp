#include "Widgets.h"
#include "imgui_internal.h"
#include <algorithm>

namespace Lgt::Editor::Widgets {

bool DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue, float columnWidth) {
    bool modified = false;
    ImGuiIO& io = ImGui::GetIO();
    auto boldFont = io.Fonts->Fonts[0]; // Assuming default font for now

    ImGui::PushID(label.c_str());

    ImGui::Columns(2);
    ImGui::SetColumnWidth(0, columnWidth);
    ImGui::Text("%s", label.c_str());
    ImGui::NextColumn();

    ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

    float lineHeight = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
    ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

    // X
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
    ImGui::PushFont(boldFont);
    if (ImGui::Button("X", buttonSize)) {
        values.x = resetValue;
        modified = true;
    }
    ImGui::PopFont();
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    if (ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f")) modified = true;
    ImGui::PopItemWidth();
    ImGui::SameLine();

    // Y
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
    ImGui::PushFont(boldFont);
    if (ImGui::Button("Y", buttonSize)) {
        values.y = resetValue;
        modified = true;
    }
    ImGui::PopFont();
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    if (ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f")) modified = true;
    ImGui::PopItemWidth();
    ImGui::SameLine();

    // Z
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
    ImGui::PushFont(boldFont);
    if (ImGui::Button("Z", buttonSize)) {
        values.z = resetValue;
        modified = true;
    }
    ImGui::PopFont();
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    if (ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f")) modified = true;
    ImGui::PopItemWidth();

    ImGui::PopStyleVar();
    ImGui::Columns(1);
    ImGui::PopID();

    return modified;
}

bool DrawColorControl(const std::string& label, glm::vec4& color) {
    bool modified = false;
    ImGui::Columns(2);
    ImGui::SetColumnWidth(0, 100.0f);
    ImGui::Text("%s", label.c_str());
    ImGui::NextColumn();
    
    ImGui::PushItemWidth(-1);
    if (ImGui::ColorEdit4(std::string("##" + label).c_str(), glm::value_ptr(color))) {
        modified = true;
    }
    ImGui::PopItemWidth();
    ImGui::Columns(1);
    return modified;
}

bool DrawComponentCardBegin(const std::string& name, bool& isExpanded, bool& isEnabled, bool canDisable) {
    ImGui::PushID(name.c_str());
    
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.12f, 0.12f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
    
    ImGui::BeginChild("##ComponentCard", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    
    bool headerClicked = false;
    ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();
    
    float lineHeight = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
    
    // Draw expand/collapse arrow
    ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    ImGui::RenderArrow(ImGui::GetWindowDrawList(), ImVec2(cursorPos.x + 4.0f, cursorPos.y + 4.0f), ImGui::GetColorU32(ImGuiCol_Text), isExpanded ? ImGuiDir_Down : ImGuiDir_Right);
    
    ImGui::SetCursorScreenPos(ImVec2(cursorPos.x + 24.0f, cursorPos.y));
    
    if (canDisable) {
        ImGui::Checkbox("##Enable", &isEnabled);
        ImGui::SameLine();
    }
    
    ImGui::Text("%s", name.c_str());
    
    // Make the header area clickable to toggle expand/collapse
    ImGui::SetCursorScreenPos(cursorPos);
    if (ImGui::InvisibleButton("##HeaderToggle", ImVec2(contentRegionAvailable.x, lineHeight))) {
        isExpanded = !isExpanded;
    }
    
    if (isExpanded) {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
    }
    
    return isExpanded;
}

void DrawComponentCardEnd() {
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
    ImGui::PopID();
    ImGui::Spacing();
}

void Sparkline(const std::string& label, const std::vector<float>& values, ImVec2 size) {
    if (values.empty()) return;
    
    ImGui::Text("%s", label.c_str());
    ImGui::SameLine(100.0f);
    
    ImVec2 pos = ImGui::GetCursorScreenPos();
    if (size.x == 0.0f) size.x = ImGui::GetContentRegionAvail().x;
    
    ImGui::Dummy(size);
    
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    // Draw background
    draw_list->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), ImColor(30, 30, 30, 255), 4.0f);
    
    if (values.size() < 2) return;
    
    float max_val = *std::max_element(values.begin(), values.end());
    float min_val = *std::min_element(values.begin(), values.end());
    
    float range = max_val - min_val;
    if (range < 0.001f) range = 0.001f;
    
    float step = size.x / (values.size() - 1);
    
    for (size_t i = 0; i < values.size() - 1; ++i) {
        float h1 = (values[i] - min_val) / range;
        float h2 = (values[i+1] - min_val) / range;
        
        ImVec2 p1 = ImVec2(pos.x + i * step, pos.y + size.y - h1 * size.y);
        ImVec2 p2 = ImVec2(pos.x + (i + 1) * step, pos.y + size.y - h2 * size.y);
        
        draw_list->AddLine(p1, p2, ImColor(100, 200, 255, 255), 2.0f);
    }
}

void LiveBadge(const std::string& label, bool active, ImU32 activeColor) {
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 textSize = ImGui::CalcTextSize(label.c_str());
    ImVec2 badgeSize = ImVec2(textSize.x + 12.0f, textSize.y + 6.0f);
    
    ImGui::Dummy(badgeSize);
    
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    ImU32 bgColor = active ? activeColor : IM_COL32(50, 50, 50, 255);
    draw_list->AddRectFilled(pos, ImVec2(pos.x + badgeSize.x, pos.y + badgeSize.y), bgColor, badgeSize.y * 0.5f);
    draw_list->AddText(ImVec2(pos.x + 6.0f, pos.y + 3.0f), ImColor(255, 255, 255, 255), label.c_str());
}

} // namespace Lgt::Editor::Widgets
