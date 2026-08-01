#pragma once
#include "Engine/Core/GlmConfig.h"
#include <cstdint>

#define ENABLE_ASSERTIONS

#ifdef _WIN32
#ifdef LIGHTVK_CORE_BUILD
#define LIGHTVK_API __declspec(dllexport)
#else
#define LIGHTVK_API __declspec(dllimport)
#endif
#else
#define LIGHTVK_API
#endif

#if defined(__clang__)
#define LGT_DEBUGBREAK() __builtin_trap()
#elif defined(__GNUC__)
#define LGT_DEBUGBREAK() __builtin_trap()
#elif defined(_MSC_VER)
#define LGT_DEBUGBREAK() __debugbreak()
#else
#include <cstdlib>
#define LGT_DEBUGBREAK() std::abort()
#endif

namespace Lgt {

#define LGT_DEFINE_HANDLE(Name)                                                                                                  \
    struct Name##Tag {};                                                                                                         \
    using Name##Handle = Handle<Name##Tag>;

// Base Handle Template
template <typename Tag> struct Handle {
public:
    using ValueType                    = uint64_t;
    static constexpr ValueType INVALID = std::numeric_limits<ValueType>::max();

    constexpr explicit Handle(ValueType key)
        : value(key) {}

    Handle() = default;

    constexpr bool IsValid() const noexcept { return value != INVALID; }
    // operators
    constexpr explicit operator bool() const noexcept { return IsValid(); }
    constexpr auto     operator<=>(const Handle&) const noexcept = default;

    // helpers
    constexpr void          Reset() noexcept { value = INVALID; }
    static constexpr Handle Invalid() noexcept { return Handle{}; }

    ValueType value = INVALID;
};

// always executes
#define LGT_ASSERT_CRITICAL(expr, msg, ...)                                                                                      \
    do {                                                                                                                         \
        if (!(expr)) {                                                                                                           \
            LIGHTVK_CRITICAL("Assertion Failed | Expr : {} | " msg, #expr, ##__VA_ARGS__);                                       \
            std::this_thread::sleep_for(std::chrono::seconds(1));                                                                \
            LGT_DEBUGBREAK();                                                                                                    \
        }                                                                                                                        \
    } while (0)

#define LGT_ASSERT_STATIC(expr, msg) static_assert(expr, msg " " #expr)

#ifdef ENABLE_ASSERTIONS

#define LGT_ASSERT(expr, msg, ...)                                                                                               \
    do {                                                                                                                         \
        if (!(expr)) {                                                                                                           \
            LIGHTVK_CRITICAL("Assertion Failed | Expr : {} | " msg, #expr, ##__VA_ARGS__);                                       \
            std::this_thread::sleep_for(std::chrono::seconds(1));                                                                \
            LGT_DEBUGBREAK();                                                                                                    \
        }                                                                                                                        \
    } while (0)

#define LIGHTVK_VERIFY(expr, msg, ...)                                                                                           \
    ([&]() {                                                                                                                     \
        auto&& result = (expr);                                                                                                  \
        if (!result) {                                                                                                           \
            LIGHTVK_ERROR("Verification Failed | Expr : {} | " msg, #expr, ##__VA_ARGS__);                                       \
            std::this_thread::sleep_for(std::chrono::seconds(1));                                                                \
            LGT_DEBUGBREAK();                                                                                                    \
        }                                                                                                                        \
        return result;                                                                                                           \
    }())

#else

#define LGT_ASSERT(expr, msg...)       (void)0
#define LIGHTVK_VERIFY(expr, msg, ...) (expr)

#endif

// GLM type aliases for consistency Temp (only
// using them for the testing/debug)
using Vec2  = glm::vec2;
using Vec3  = glm::vec3;
using Vec4  = glm::vec4;
using Mat3  = glm::mat3;
using Mat4  = glm::mat4;
using IVec2 = glm::ivec2;
using IVec3 = glm::ivec3;
using IVec4 = glm::ivec4;
using UVec2 = glm::uvec2;
using UVec3 = glm::uvec3;
using UVec4 = glm::uvec4;
using Quat  = glm::quat;

using u8  = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using i8  = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;
using f32 = float;
using f64 = double;

} // namespace Lgt