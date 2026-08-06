#include "ProfilerPanel.h"
#include "Engine/Core/Profiler.h"
#include <imgui.h>
#include <algorithm>
#include <cmath>

namespace Lgt::Editor::Panel {

void ProfilerPanel::Init(Context* context) {
    _context     = context;
    _lastRefresh = Clock::now();
}

void ProfilerPanel::Shutdown() {}

void ProfilerPanel::Draw() {
    ImGui::Begin("Profiler");

#if LGT_PROFILING_ENABLED

    // ── Refresh throttle ────────────────────────────────────────────────
    auto now     = Clock::now();
    auto elapsed = std::chrono::duration<double, std::milli>(now - _lastRefresh).count();

    if (!_paused && elapsed >= _refreshIntervalMs) {
        _lastRefresh = now;
        _cachedFrameMs = Profiler::GetLastFrameTimeMs();
        _cachedFps     = (_cachedFrameMs > 0.0) ? 1000.0 / _cachedFrameMs : 0.0;
        _cachedZones   = Profiler::GetZones(); // copy snapshot
    }

    // ── Toolbar ─────────────────────────────────────────────────────────
    if (ImGui::Button(_paused ? "Resume" : "Pause")) {
        _paused = !_paused;
    }
    ImGui::SameLine();

    ImGui::SetNextItemWidth(120);
    ImGui::SliderFloat("Refresh", &_refreshIntervalMs, 50.0f, 1000.0f, "%.0f ms");
    ImGui::SameLine();

    ImGui::Text("Frame: %.2f ms  |  FPS: %.0f", _cachedFrameMs, _cachedFps);

    ImGui::Separator();
    ImGui::Spacing();

    // ── Frame Time Graph ────────────────────────────────────────────────
    // The graph always reads live data (it's already a ring buffer so it's smooth)
    ImGui::Text("Frame Time History");

    const float* history = Profiler::GetFrameTimeHistory();
    u32          count   = Profiler::GetFrameTimeCount();
    u32          head    = Profiler::GetFrameTimeHead();

    if (count > 0) {
        static float plotBuffer[Profiler::HISTORY_SIZE];
        u32 plotCount = std::min(count, Profiler::HISTORY_SIZE);

        for (u32 i = 0; i < plotCount; ++i) {
            u32 idx      = (head - plotCount + i) % Profiler::HISTORY_SIZE;
            plotBuffer[i] = history[idx];
        }

        float maxVal = *std::max_element(plotBuffer, plotBuffer + plotCount);
        maxVal       = std::max(maxVal, 1.0f);

        char overlay[64];
        snprintf(overlay, sizeof(overlay), "%.1f ms", plotBuffer[plotCount - 1]);

        ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.08f, 0.08f, 0.12f, 1.0f));
        ImGui::PlotLines("##FrameTime", plotBuffer, static_cast<int>(plotCount), 0, overlay, 0.0f, maxVal * 1.2f,
                         ImVec2(-1, 80));
        ImGui::PopStyleColor(2);

        ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.0f), "16.6ms (60fps)");
        ImGui::SameLine(200);
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.3f, 1.0f), "33.3ms (30fps)");
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ── Zone List (uses cached snapshot) ────────────────────────────────
    ImGui::Text("Profile Zones (Last Snapshot)");
    ImGui::Spacing();

    if (_cachedZones.empty()) {
        ImGui::TextDisabled("No zones recorded");
    } else {
        if (ImGui::BeginTable("##Zones", 3,
                              ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("Zone", ImGuiTableColumnFlags_None, 3.0f);
            ImGui::TableSetupColumn("Duration", ImGuiTableColumnFlags_None, 1.0f);
            ImGui::TableSetupColumn("% Frame", ImGuiTableColumnFlags_None, 1.0f);
            ImGui::TableHeadersRow();

            for (const auto& zone : _cachedZones) {
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                std::string indent(zone.depth * 2, ' ');
                ImGui::Text("%s%s", indent.c_str(), zone.name.c_str());

                ImGui::TableSetColumnIndex(1);
                if (zone.durationMs < 1.0) {
                    ImGui::Text("%.0f us", zone.durationMs * 1000.0);
                } else {
                    ImGui::Text("%.2f ms", zone.durationMs);
                }

                ImGui::TableSetColumnIndex(2);
                float pct = (_cachedFrameMs > 0.0) ? static_cast<float>(zone.durationMs / _cachedFrameMs * 100.0) : 0.0f;

                ImVec4 color;
                if (pct < 10.0f)
                    color = ImVec4(0.4f, 0.9f, 0.4f, 1.0f);
                else if (pct < 50.0f)
                    color = ImVec4(0.9f, 0.9f, 0.3f, 1.0f);
                else
                    color = ImVec4(0.9f, 0.3f, 0.3f, 1.0f);

                ImGui::TextColored(color, "%.1f%%", pct);
            }

            ImGui::EndTable();
        }
    }

#else
    ImGui::TextDisabled("Profiling is disabled in Release builds.");
    ImGui::TextDisabled("Build in Debug mode to enable the profiler.");
#endif

    ImGui::End();
}

} // namespace Lgt::Editor::Panel
