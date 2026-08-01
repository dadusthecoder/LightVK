#pragma once
#include <string>
#include <vector>
#include "imgui.h"
#include "Engine/Core/Core.h"

namespace Lgt::Editor::Widgets {

// Draws a premium Vector3 editor with X, Y, Z colored labels and drag functionality
bool DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue = 0.0f, float columnWidth = 100.0f);

// Draws a premium Vector4/Color editor
bool DrawColorControl(const std::string& label, glm::vec4& color);

// Draws a card-like header for components
bool DrawComponentCardBegin(const std::string& name, bool& isExpanded, bool& isEnabled, bool canDisable = true);
void DrawComponentCardEnd();

// Draws a live sparkline graph for performance or memory tracking
void Sparkline(const std::string& label, const std::vector<float>& values, ImVec2 size = ImVec2(0, 40));

// Draws an animated status badge
void LiveBadge(const std::string& label, bool active, ImU32 activeColor = IM_COL32(0, 255, 0, 255));

} // namespace Lgt::Editor::Widgets
