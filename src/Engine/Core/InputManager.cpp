// src/core/InputManager.cpp
#include "Engine/Core/InputManager.h"
#include <GLFW/glfw3.h>

namespace Lgt {

float InputManager::s_ScrollAccum = 0.f;

void InputManager::ScrollCallback(GLFWwindow*, double, double yOffset) {
    s_ScrollAccum += static_cast<float>(yOffset);
}

InputManager::InputManager(GLFWwindow* window)
    : _window(window) {
    glfwSetScrollCallback(window, ScrollCallback);
    _keyCurr.fill(false);
    _keyPrev.fill(false);
    _btnCurr.fill(false);
    _btnPrev.fill(false);

    SetDefaultBindings();
}

void InputManager::SetDefaultBindings() {
    BindKey(InputAction::MoveForward, Key::W);
    BindKey(InputAction::MoveBackward, Key::S);
    BindKey(InputAction::MoveLeft, Key::A);
    BindKey(InputAction::MoveRight, Key::D);
    BindKey(InputAction::Jump, Key::Space);
    BindKey(InputAction::Sprint, Key::LeftShift);
    BindKey(InputAction::Crouch, Key::LeftControl);
    BindKey(InputAction::Interact, Key::E);
    BindKey(InputAction::ToggleUI, Key::Tab);

    BindMouseButton(InputAction::Fire, Mouse::Left);
    BindMouseButton(InputAction::AltFire, Mouse::Right);

    // Common gamepad bindings (assuming standard GLFW gamepad mapping)
    BindGamepadButton(InputAction::Jump, GLFW_GAMEPAD_BUTTON_A);
    BindGamepadButton(InputAction::Sprint, GLFW_GAMEPAD_BUTTON_LEFT_THUMB); // L3
    BindGamepadButton(InputAction::Crouch, GLFW_GAMEPAD_BUTTON_B);
    BindGamepadButton(InputAction::Interact, GLFW_GAMEPAD_BUTTON_X);
}

void InputManager::ResetFrame() {
    // Snapshot previous state
    _keyPrev = _keyCurr;
    _btnPrev = _btnCurr;
    _gpBtnPrev = _gpBtnCurr;

    // Poll keys
    for (int k = GLFW_KEY_SPACE; k <= GLFW_KEY_LAST; ++k)
        _keyCurr[k] = glfwGetKey(_window, k) == GLFW_PRESS;

    // Poll mouse buttons
    for (int b = 0; b < kMaxButtons; ++b)
        _btnCurr[b] = glfwGetMouseButton(_window, b) == GLFW_PRESS;

    // Gamepad
    GLFWgamepadstate state;
    if (glfwJoystickIsGamepad(GLFW_JOYSTICK_1) && glfwGetGamepadState(GLFW_JOYSTICK_1, &state)) {
        _gamepadConnected = true;
        for (int i = 0; i < kMaxGamepadButtons; ++i) {
            _gpBtnCurr[i] = state.buttons[i] == GLFW_PRESS;
        }
        for (int i = 0; i < kMaxGamepadAxes; ++i) {
            _gpAxes[i] = state.axes[i];
        }
    } else {
        _gamepadConnected = false;
        _gpBtnCurr.fill(false);
        _gpAxes.fill(0.0f);
    }

    // Mouse position + delta
    double mx = 0.0, my = 0.0;
    glfwGetCursorPos(_window, &mx, &my);
    glm::vec2 newPos = {static_cast<f32>(mx), static_cast<f32>(my)};
    _mouseDelta     = _cursorCaptured ? (newPos - _mousePrev) : glm::vec2(0.f);
    _mousePrev      = newPos;
    _mousePos       = newPos;

    // Consume scroll
    _scrollDelta = s_ScrollAccum;
    s_ScrollAccum = 0.f;
}

bool InputManager::IsKeyDown(int key) const {
    return key >= 0 && key < kMaxKeys && _keyCurr[key];
}
bool InputManager::WasKeyPressed(int key) const {
    return key >= 0 && key < kMaxKeys && _keyCurr[key] && !_keyPrev[key];
}
bool InputManager::WasKeyReleased(int key) const {
    return key >= 0 && key < kMaxKeys && !_keyCurr[key] && _keyPrev[key];
}

bool InputManager::IsMouseDown(int b) const {
    return b >= 0 && b < kMaxButtons && _btnCurr[b];
}
bool InputManager::WasMousePressed(int b) const {
    return b >= 0 && b < kMaxButtons && _btnCurr[b] && !_btnPrev[b];
}

void InputManager::SetCursorCaptured(bool captured) {
    _cursorCaptured = captured;
    glfwSetInputMode(_window, GLFW_CURSOR, captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    // Reset delta so we don't get a jump on first captured frame
    double mx = 0.0, my = 0.0;
    glfwGetCursorPos(_window, &mx, &my);
    _mousePrev = {static_cast<f32>(mx), static_cast<f32>(my)};
}

void InputManager::BindKey(InputAction action, int key) {
    if (static_cast<size_t>(action) < _bindings.size()) {
        _bindings[static_cast<size_t>(action)].key = key;
    }
}

void InputManager::BindMouseButton(InputAction action, int button) {
    if (static_cast<size_t>(action) < _bindings.size()) {
        _bindings[static_cast<size_t>(action)].mouseButton = button;
    }
}

void InputManager::BindGamepadButton(InputAction action, int button) {
    if (static_cast<size_t>(action) < _bindings.size()) {
        _bindings[static_cast<size_t>(action)].gamepadButton = button;
    }
}

void InputManager::BindGamepadAxis(InputAction action, int axis, float deadzone, bool invert) {
    if (static_cast<size_t>(action) < _bindings.size()) {
        auto& binding = _bindings[static_cast<size_t>(action)];
        binding.gamepadAxis = axis;
        binding.axisDeadzone = deadzone;
        binding.axisInvert = invert;
    }
}

bool InputManager::IsActionDown(InputAction action) const {
    if (static_cast<size_t>(action) >= _bindings.size()) return false;
    const auto& binding = _bindings[static_cast<size_t>(action)];
    
    if (binding.key != -1 && IsKeyDown(binding.key)) return true;
    if (binding.mouseButton != -1 && IsMouseDown(binding.mouseButton)) return true;
    if (_gamepadConnected && binding.gamepadButton != -1 && binding.gamepadButton < kMaxGamepadButtons) {
        if (_gpBtnCurr[binding.gamepadButton]) return true;
    }
    return false;
}

bool InputManager::WasActionPressed(InputAction action) const {
    if (static_cast<size_t>(action) >= _bindings.size()) return false;
    const auto& binding = _bindings[static_cast<size_t>(action)];
    
    if (binding.key != -1 && WasKeyPressed(binding.key)) return true;
    if (binding.mouseButton != -1 && WasMousePressed(binding.mouseButton)) return true;
    if (_gamepadConnected && binding.gamepadButton != -1 && binding.gamepadButton < kMaxGamepadButtons) {
        if (_gpBtnCurr[binding.gamepadButton] && !_gpBtnPrev[binding.gamepadButton]) return true;
    }
    return false;
}

bool InputManager::WasActionReleased(InputAction action) const {
    if (static_cast<size_t>(action) >= _bindings.size()) return false;
    const auto& binding = _bindings[static_cast<size_t>(action)];
    
    if (binding.key != -1 && WasKeyReleased(binding.key)) return true;
    if (binding.mouseButton != -1 && !_btnCurr[binding.mouseButton] && _btnPrev[binding.mouseButton]) return true;
    if (_gamepadConnected && binding.gamepadButton != -1 && binding.gamepadButton < kMaxGamepadButtons) {
        if (!_gpBtnCurr[binding.gamepadButton] && _gpBtnPrev[binding.gamepadButton]) return true;
    }
    return false;
}

float InputManager::GetActionAxis(InputAction action) const {
    if (static_cast<size_t>(action) >= _bindings.size()) return 0.0f;
    const auto& binding = _bindings[static_cast<size_t>(action)];
    
    if (!_gamepadConnected || binding.gamepadAxis == -1 || binding.gamepadAxis >= kMaxGamepadAxes) {
        return 0.0f;
    }
    
    float val = _gpAxes[binding.gamepadAxis];
    if (std::abs(val) < binding.axisDeadzone) {
        return 0.0f;
    }
    
    float sign = val > 0.0f ? 1.0f : -1.0f;
    val = sign * (std::abs(val) - binding.axisDeadzone) / (1.0f - binding.axisDeadzone);
    
    return binding.axisInvert ? -val : val;
}

} // namespace Lgt
