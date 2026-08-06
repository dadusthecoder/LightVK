#include "Profiler.h"

#if LGT_PROFILING_ENABLED

namespace Lgt {

void Profiler::BeginFrame() {
    _zonesLastFrame = std::move(_zones);
    _zones.clear();
    _zoneStack.clear();
    _currentDepth = 0;
    _frameStart   = Clock::now();
}

void Profiler::EndFrame() {
    auto end         = Clock::now();
    _lastFrameTimeMs = std::chrono::duration<double, std::milli>(end - _frameStart).count();

    _frameTimeHistory[_frameTimeHead % HISTORY_SIZE] = static_cast<float>(_lastFrameTimeMs);
    _frameTimeHead++;
    if (_frameTimeCount < HISTORY_SIZE) {
        _frameTimeCount++;
    }
}

void Profiler::BeginZone(const std::string& name) {
    ActiveZone zone;
    zone.name  = name;
    zone.start = Clock::now();
    zone.depth = _currentDepth++;
    _zoneStack.push_back(std::move(zone));
}

void Profiler::EndZone() {
    if (_zoneStack.empty())
        return;

    auto        end        = Clock::now();
    ActiveZone& activeZone = _zoneStack.back();

    ProfileZone zone;
    zone.name       = std::move(activeZone.name);
    zone.startMs    = std::chrono::duration<double, std::milli>(activeZone.start - _frameStart).count();
    zone.durationMs = std::chrono::duration<double, std::milli>(end - activeZone.start).count();
    zone.depth      = activeZone.depth;

    _zones.push_back(std::move(zone));
    _zoneStack.pop_back();
    _currentDepth--;
}

const std::vector<ProfileZone>& Profiler::GetZones() {
    return _zonesLastFrame;
}

const float* Profiler::GetFrameTimeHistory() {
    return _frameTimeHistory;
}

u32 Profiler::GetFrameTimeCount() {
    return _frameTimeCount;
}

u32 Profiler::GetFrameTimeHead() {
    return _frameTimeHead;
}

double Profiler::GetLastFrameTimeMs() {
    return _lastFrameTimeMs;
}

} // namespace Lgt

#endif // LGT_PROFILING_ENABLED
