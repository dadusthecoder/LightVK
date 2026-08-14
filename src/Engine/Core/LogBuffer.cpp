#include "LogBuffer.h"

namespace Lgt {

std::mutex LogBuffer::_mutex;
std::array<LogEntry, LogBuffer::MAX_ENTRIES> LogBuffer::_entries;
uint32_t LogBuffer::_head = 0;
uint32_t LogBuffer::_count = 0;

void LogBuffer::Push(spdlog::level::level_enum level, const std::string& message, const std::string& timestamp) {
    std::lock_guard<std::mutex> lock(_mutex);
    auto& entry = _entries[_head % MAX_ENTRIES];
    entry.level = level;
    entry.message = message;
    entry.timestamp = timestamp;
    _head++;
    if (_count < MAX_ENTRIES) _count++;
}

void LogBuffer::Clear() {
    std::lock_guard<std::mutex> lock(_mutex);
    _head = 0;
    _count = 0;
}

uint32_t LogBuffer::Count() {
    std::lock_guard<std::mutex> lock(_mutex);
    return _count;
}

} // namespace Lgt
