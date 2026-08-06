#pragma once
#include "Engine/Core/Core.h"
#include <string>
#include <vector>
#include <chrono>

namespace Lgt {

// ── Profiling is only active in Debug builds ────────────────────────────────
// In Release mode, all profiler calls and macros compile to nothing.

#ifndef NDEBUG
#define LGT_PROFILING_ENABLED 1
#else
#define LGT_PROFILING_ENABLED 0
#endif

struct ProfileZone {
    std::string name;
    double startMs    = 0.0;
    double durationMs = 0.0;
    u32    depth      = 0;
};

#if LGT_PROFILING_ENABLED

class Profiler {
public:
    static constexpr u32 HISTORY_SIZE = 256;

    static void BeginFrame();
    static void EndFrame();

    static void BeginZone(const std::string& name);
    static void EndZone();

    // Last frame's completed zones
    static const std::vector<ProfileZone>& GetZones();

    // Ring buffer of frame times (ms)
    static const float* GetFrameTimeHistory();
    static u32          GetFrameTimeCount();
    static u32          GetFrameTimeHead();
    static double       GetLastFrameTimeMs();

private:
    using Clock     = std::chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;

    struct ActiveZone {
        std::string name;
        TimePoint   start;
        u32         depth;
    };

    static inline TimePoint _frameStart;
    static inline double    _lastFrameTimeMs = 0.0;
    static inline float     _frameTimeHistory[HISTORY_SIZE] = {};
    static inline u32       _frameTimeHead  = 0;
    static inline u32       _frameTimeCount = 0;

    static inline std::vector<ProfileZone> _zones;
    static inline std::vector<ProfileZone> _zonesLastFrame;
    static inline std::vector<ActiveZone>  _zoneStack;
    static inline u32                      _currentDepth = 0;
};

class ScopedZone {
public:
    explicit ScopedZone(const std::string& name) { Profiler::BeginZone(name); }
    ~ScopedZone() { Profiler::EndZone(); }
    ScopedZone(const ScopedZone&)            = delete;
    ScopedZone& operator=(const ScopedZone&) = delete;
};

#define LGT_PROFILE_SCOPE(name) ::Lgt::ScopedZone _profileZone##__LINE__(name)
#define LGT_PROFILE_FUNCTION()  LGT_PROFILE_SCOPE(__func__)

#else // Release mode — everything compiles to nothing

class Profiler {
public:
    static constexpr u32 HISTORY_SIZE = 256;

    static void   BeginFrame() {}
    static void   EndFrame() {}
    static void   BeginZone(const std::string&) {}
    static void   EndZone() {}

    static const std::vector<ProfileZone>& GetZones() {
        static std::vector<ProfileZone> empty;
        return empty;
    }
    static const float* GetFrameTimeHistory() {
        static float empty[HISTORY_SIZE] = {};
        return empty;
    }
    static u32    GetFrameTimeCount()     { return 0; }
    static u32    GetFrameTimeHead()      { return 0; }
    static double GetLastFrameTimeMs()    { return 0.0; }
};

#define LGT_PROFILE_SCOPE(name) (void)0
#define LGT_PROFILE_FUNCTION()  (void)0

#endif // LGT_PROFILING_ENABLED

} // namespace Lgt
