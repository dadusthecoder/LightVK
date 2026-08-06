#pragma once

#include "Core.h"
#include "Engine/Core/GlmConfig.h"
#include <array>
#include <cmath>

struct GLFWwindow;

namespace Lgt {

// Re-export GLFW key codes without exposing GLFW headers to consumers
// (values deliberately match GLFW so no translation is needed at runtime)
namespace Key {
static constexpr int Unknown = -1;
static constexpr int Space   = 32;
static constexpr int A = 65, B = 66, C = 67, D = 68, E = 69, F = 70;
static constexpr int G = 71, H = 72, I = 73, J = 74, K = 75, L = 76;
static constexpr int M = 77, N = 78, O = 79, P = 80, Q = 81, R = 82;
static constexpr int S = 83, T = 84, U = 85, V = 86, W = 87, X = 88;
static constexpr int Y = 89, Z = 90;
static constexpr int Escape = 256;
static constexpr int Enter  = 257;
static constexpr int Tab    = 258;
static constexpr int Left = 263, Right = 262, Up = 265, Down = 264;
static constexpr int LeftShift   = 340;
static constexpr int LeftControl = 341;
static constexpr int LeftAlt     = 342;
static constexpr int F1 = 290, F2 = 291, F3 = 292, F11 = 300;
} // namespace Key

namespace Mouse {
static constexpr int Left   = 0;
static constexpr int Right  = 1;
static constexpr int Middle = 2;
} // namespace Mouse

// Named input actions for game-agnostic binding
enum class InputAction : uint32_t {
    MoveForward = 0,
    MoveBackward,
    MoveLeft,
    MoveRight,
    MoveUp,
    MoveDown,
    Jump,
    Sprint,
    Crouch,
    Fire,
    AltFire,
    Interact,
    ToggleUI,
    CameraLook,  // axis action
    COUNT
};

struct ActionBinding {
    int key           = -1;   // GLFW key code, -1 = unbound
    int mouseButton   = -1;   // GLFW mouse button, -1 = unbound
    int gamepadButton = -1;   // GLFW gamepad button, -1 = unbound
    int gamepadAxis   = -1;   // GLFW gamepad axis, -1 = unbound
    float axisDeadzone = 0.15f;
    bool  axisInvert   = false;
};

class InputManager {
public:
    explicit InputManager(GLFWwindow* window);

    // Call once at start of frame, before any IsKey* queries
    void ResetFrame();

    // Held this frame
    [[nodiscard]] bool IsKeyDown(int key) const;
    // Pressed this frame (down now, was up last frame)
    [[nodiscard]] bool WasKeyPressed(int key) const;
    // Released this frame
    [[nodiscard]] bool WasKeyReleased(int key) const;

    [[nodiscard]] bool IsMouseDown(int button) const;
    [[nodiscard]] bool WasMousePressed(int button) const;

    // Mouse position in window pixels
    [[nodiscard]] glm::vec2 GetMousePos() const { return _mousePos; }
    // Delta since last frame (only non-zero when cursor is captured)
    [[nodiscard]] glm::vec2 GetMouseDelta() const { return _mouseDelta; }
    [[nodiscard]] f32       GetScrollDelta() const { return _scrollDelta; }

    // Lock/unlock cursor (for FPS-style look)
    void               SetCursorCaptured(bool captured);
    [[nodiscard]] bool IsCursorCaptured() const { return _cursorCaptured; }

    // Action mapping
    void BindKey(InputAction action, int key);
    void BindMouseButton(InputAction action, int button);
    void BindGamepadButton(InputAction action, int button);
    void BindGamepadAxis(InputAction action, int axis, float deadzone = 0.15f, bool invert = false);

    [[nodiscard]] bool  IsActionDown(InputAction action) const;
    [[nodiscard]] bool  WasActionPressed(InputAction action) const;
    [[nodiscard]] bool  WasActionReleased(InputAction action) const;
    [[nodiscard]] float GetActionAxis(InputAction action) const;

    // Gamepad
    [[nodiscard]] bool IsGamepadConnected() const { return _gamepadConnected; }

    // Set up default bindings
    void SetDefaultBindings();

private:
    static void ScrollCallback(GLFWwindow*, double, double yOffset);

    GLFWwindow* _window;

    static constexpr int kMaxKeys    = 512;
    static constexpr int kMaxButtons = 8;

    std::array<bool, kMaxKeys>    _keyCurr{};
    std::array<bool, kMaxKeys>    _keyPrev{};
    std::array<bool, kMaxButtons> _btnCurr{};
    std::array<bool, kMaxButtons> _btnPrev{};

    glm::vec2 _mousePos       = {};
    glm::vec2 _mousePrev      = {};
    glm::vec2 _mouseDelta     = {};
    f32       _scrollDelta    = 0.f;
    bool      _cursorCaptured = false;

    // Action bindings
    std::array<ActionBinding, static_cast<size_t>(InputAction::COUNT)> _bindings{};

    // Gamepad state (mirrors GLFWgamepadstate)
    static constexpr int kMaxGamepadButtons = 15;
    static constexpr int kMaxGamepadAxes = 6;
    bool _gamepadConnected = false;
    std::array<bool, kMaxGamepadButtons> _gpBtnCurr{};
    std::array<bool, kMaxGamepadButtons> _gpBtnPrev{};
    std::array<float, kMaxGamepadAxes>   _gpAxes{};

    // Accumulated scroll between frames (written by GLFW callback)
    static float s_ScrollAccum;
};

} // namespace Lgt
