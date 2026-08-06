#include "Engine/Core/Logger.h"
#include "Engine/Core/LogBuffer.h"
#include <chrono>
#include <unordered_map>

// Rate-limited sink - prevents console flooding
class RateLimitedSink : public spdlog::sinks::base_sink<std::mutex> {
public:
    explicit RateLimitedSink(std::shared_ptr<spdlog::sinks::sink> wrapped_sink,
                             std::chrono::milliseconds            rate_limit_interval = std::chrono::milliseconds(100))
        : _wrapped_sink(wrapped_sink),
          _rate_limit_interval(rate_limit_interval) {}

    void set_rate_limit(std::chrono::milliseconds interval) {
        std::lock_guard<std::mutex> lock(base_sink<std::mutex>::mutex_);
        _rate_limit_interval = interval;
    }

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        auto        now     = std::chrono::steady_clock::now();
        std::string msg_key = std::string(msg.payload.data(), msg.payload.size());

        auto it = _last_log_time.find(msg_key);

        // Always allow errors and critical
        if (msg.level >= spdlog::level::err) {
            _wrapped_sink->log(msg);
            _last_log_time[msg_key] = now;
            return;
        }

        // Rate limit other messages
        if (it == _last_log_time.end() || (now - it->second) >= _rate_limit_interval) {
            auto suppress_it = _suppressed_count.find(msg_key);
            if (suppress_it != _suppressed_count.end() && suppress_it->second > 0) {
                spdlog::details::log_msg modified_msg     = msg;
                std::string              modified_payload = fmt::format("{} (+{} suppressed)", msg_key, suppress_it->second);
                modified_msg.payload                      = modified_payload;
                _wrapped_sink->log(modified_msg);
                _suppressed_count[msg_key] = 0;
            } else {
                _wrapped_sink->log(msg);
            }

            _last_log_time[msg_key] = now;
        } else {
            _suppressed_count[msg_key]++;
        }
    }

    void flush_() override { _wrapped_sink->flush(); }

    void set_pattern_(const std::string& pattern) override { _wrapped_sink->set_pattern(pattern); }

    void set_formatter_(std::unique_ptr<spdlog::formatter> sink_formatter) override {
        _wrapped_sink->set_formatter(std::move(sink_formatter));
    }

private:
    std::shared_ptr<spdlog::sinks::sink>                                   _wrapped_sink;
    std::chrono::milliseconds                                              _rate_limit_interval;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> _last_log_time;
    std::unordered_map<std::string, uint32_t>                              _suppressed_count;
};

// Custom sink with level-specific patterns
class LevelPatternSink : public spdlog::sinks::base_sink<std::mutex> {
public:
    explicit LevelPatternSink(std::shared_ptr<spdlog::sinks::sink> wrapped_sink)
        : _wrapped_sink(wrapped_sink) {
        _patterns[spdlog::level::trace]    = "[%T] [TRACE] %v";
        _patterns[spdlog::level::debug]    = "[%T] [DEBUG] %v";
        _patterns[spdlog::level::info]     = "%^[%T] %v%$";
        _patterns[spdlog::level::warn]     = "%^[%T] [WARN] [%!:%#] %v%$";
        _patterns[spdlog::level::err]      = "%^[%T] [ERROR] [%!:%#] %v%$";
        _patterns[spdlog::level::critical] = "%^[%T] [CRITICAL] [%!:%#] %v%$";
    }

    void set_pattern_for_level(spdlog::level::level_enum level, const std::string& pattern) {
        std::lock_guard<std::mutex> lock(base_sink<std::mutex>::mutex_);
        _patterns[level] = pattern;
    }

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        auto it = _patterns.find(msg.level);
        if (it != _patterns.end()) {
            _wrapped_sink->set_pattern(it->second);
        }
        _wrapped_sink->log(msg);
    }

    void flush_() override { _wrapped_sink->flush(); }

    void set_pattern_(const std::string& pattern) override { _wrapped_sink->set_pattern(pattern); }

    void set_formatter_(std::unique_ptr<spdlog::formatter> sink_formatter) override {
        _wrapped_sink->set_formatter(std::move(sink_formatter));
    }

private:
    std::shared_ptr<spdlog::sinks::sink>                       _wrapped_sink;
    std::unordered_map<spdlog::level::level_enum, std::string> _patterns;
};

std::shared_ptr<spdlog::logger> Logger::s_CoreLogger;

std::shared_ptr<spdlog::logger>& Logger::Core() {
    return s_CoreLogger;
}

// Sink that feeds into the in-editor console panel
class EditorConsoleSink : public spdlog::sinks::base_sink<std::mutex> {
protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        // Format timestamp
        auto time = std::chrono::system_clock::to_time_t(msg.time);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(msg.time.time_since_epoch()) % 1000;
        std::tm tm_buf;
        localtime_s(&tm_buf, &time);
        char timeBuf[32];
        snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d.%03d", tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec, (int)ms.count());

        std::string payload(msg.payload.data(), msg.payload.size());
        Lgt::LogBuffer::Push(msg.level, payload, std::string(timeBuf));
    }
    void flush_() override {}
};

void Logger::Init() {
    spdlog::init_thread_pool(8192, 1);

    // Console sink with rate limiting and level-specific patterns
    auto baseConsoleSink        = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto rateLimitedConsoleSink = std::make_shared<RateLimitedSink>(baseConsoleSink, std::chrono::milliseconds(100));
    auto consoleSink            = std::make_shared<LevelPatternSink>(rateLimitedConsoleSink);
    consoleSink->set_level(spdlog::level::trace);

    // Customize console patterns
    consoleSink->set_pattern_for_level(spdlog::level::trace, "%^[%n]%v%$");
    consoleSink->set_pattern_for_level(spdlog::level::debug, "%^[%n]%v%$");
    consoleSink->set_pattern_for_level(spdlog::level::info, "%^[%n]%v%$");
    consoleSink->set_pattern_for_level(spdlog::level::warn, "%^[%n]%v%$");
    consoleSink->set_pattern_for_level(spdlog::level::err, "%^[%n]%v%$");
    consoleSink->set_pattern_for_level(spdlog::level::critical, "%^[%n]%v%$");

    // File sink with detailed patterns (no rate limiting for files)
    auto baseFileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/LightVK.log", true);
    auto fileSink     = std::make_shared<LevelPatternSink>(baseFileSink);
    fileSink->set_level(spdlog::level::trace);

    // Detailed file patterns
    fileSink->set_pattern_for_level(spdlog::level::trace, "[%Y-%m-%d %T.%e] [T] [thread %t] [%n] %v");
    fileSink->set_pattern_for_level(spdlog::level::debug, "[%Y-%m-%d %T.%e] [D] [thread %t] [%n] [%!] %v");
    fileSink->set_pattern_for_level(spdlog::level::info, "[%Y-%m-%d %T.%e] [I] [thread %t] [%n] %v");
    fileSink->set_pattern_for_level(spdlog::level::warn, "[%Y-%m-%d %T.%e] [W] [thread %t] [%n] [%!:%#] %v");
    fileSink->set_pattern_for_level(spdlog::level::err, "[%Y-%m-%d %T.%e] [E] [thread %t] [%n] [%s:%#] [%!] %v");
    fileSink->set_pattern_for_level(spdlog::level::critical, "[%Y-%m-%d %T.%e] [C] [thread %t] [%n] [%s:%#] [%!] %v");

    auto editorSink = std::make_shared<EditorConsoleSink>();
    editorSink->set_level(spdlog::level::trace);

    s_CoreLogger = std::make_shared<spdlog::async_logger>(
        "LIGHTVK", spdlog::sinks_init_list{consoleSink, fileSink, editorSink}, spdlog::thread_pool(), spdlog::async_overflow_policy::block);

    s_CoreLogger->set_level(spdlog::level::trace);
    s_CoreLogger->flush_on(spdlog::level::err);
    spdlog::register_logger(s_CoreLogger);
}

void Logger::Shutdown() {
    if (s_CoreLogger)
        s_CoreLogger->flush();

    s_CoreLogger.reset();
    spdlog::shutdown();
}

void Logger::LogStatus(const std::string& msg) {
    if (!s_CoreLogger)
        return;

    auto& sinks = s_CoreLogger->sinks();
    if (sinks.empty())
        return;

    static size_t last_len = 0;

    std::string padded = msg;

    // Clear leftovers from previous longer message
    if (last_len > msg.size())
        padded += std::string(last_len - msg.size(), ' ');

    last_len = padded.size();

    spdlog::details::log_msg logMsg(s_CoreLogger->name(), spdlog::level::info, "\r" + padded);

    sinks[0]->log(logMsg);
    sinks[0]->flush();
}
