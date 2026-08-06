#pragma once
#include "Editor/Context.h"
#include "Engine/Core/Profiler.h"
#include <chrono>
#include <vector>

namespace Lgt::Editor::Panel {

class ProfilerPanel {
public:
    void Init(Context* context);
    void Shutdown();
    void Draw();

private:
    Context* _context = nullptr;
    bool     _paused  = false;

    // Refresh throttle — snapshot updates every _refreshIntervalMs
    float _refreshIntervalMs = 200.0f;

    using Clock = std::chrono::steady_clock;
    Clock::time_point            _lastRefresh{};
    double                       _cachedFrameMs = 0.0;
    double                       _cachedFps     = 0.0;
    std::vector<ProfileZone>     _cachedZones;
};

} // namespace Lgt::Editor::Panel
