#pragma once
#include <string>
#include <mutex>
#include <array>
#include <cstdint>
#include <spdlog/spdlog.h>

namespace Lgt {

struct LogEntry {
    spdlog::level::level_enum level = spdlog::level::info;
    std::string message;
    std::string timestamp;
};

class LogBuffer {
public:
    static constexpr uint32_t MAX_ENTRIES = 2048;

    static void Push(spdlog::level::level_enum level, const std::string& message, const std::string& timestamp) {
        std::lock_guard<std::mutex> lock(_mutex);
        auto& entry = _entries[_head % MAX_ENTRIES];
        entry.level = level;
        entry.message = message;
        entry.timestamp = timestamp;
        _head++;
        if (_count < MAX_ENTRIES) _count++;
    }

    static void Clear() {
        std::lock_guard<std::mutex> lock(_mutex);
        _head = 0;
        _count = 0;
    }

    // Thread-safe snapshot for reading
    template<typename Fn>
    static void ForEach(Fn&& fn) {
        std::lock_guard<std::mutex> lock(_mutex);
        uint32_t start = (_count < MAX_ENTRIES) ? 0 : (_head % MAX_ENTRIES);
        for (uint32_t i = 0; i < _count; ++i) {
            uint32_t idx = (start + i) % MAX_ENTRIES;
            fn(_entries[idx]);
        }
    }

    static uint32_t Count() {
        std::lock_guard<std::mutex> lock(_mutex);
        return _count;
    }

private:
    static inline std::mutex _mutex;
    static inline std::array<LogEntry, MAX_ENTRIES> _entries;
    static inline uint32_t _head = 0;
    static inline uint32_t _count = 0;
};

} // namespace Lgt
