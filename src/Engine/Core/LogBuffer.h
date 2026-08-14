#pragma once
#include <string>
#include <mutex>
#include <array>
#include <cstdint>
#include <spdlog/spdlog.h>

#include "Engine/Core/Core.h"

namespace Lgt {

struct LIGHTVK_API LogEntry {
    spdlog::level::level_enum level = spdlog::level::info;
    std::string message;
    std::string timestamp;
};

class LIGHTVK_API LogBuffer {
public:
    static constexpr uint32_t MAX_ENTRIES = 2048;

    static void Push(spdlog::level::level_enum level, const std::string& message, const std::string& timestamp);
    static void Clear();
    
    template<typename Fn>
    static void ForEach(Fn&& fn) {
        std::lock_guard<std::mutex> lock(_mutex);
        uint32_t start = (_count < MAX_ENTRIES) ? 0 : (_head % MAX_ENTRIES);
        for (uint32_t i = 0; i < _count; ++i) {
            uint32_t idx = (start + i) % MAX_ENTRIES;
            fn(_entries[idx]);
        }
    }

    static uint32_t Count();

private:
    static std::mutex _mutex;
    static std::array<LogEntry, MAX_ENTRIES> _entries;
    static uint32_t _head;
    static uint32_t _count;
};

} // namespace Lgt
