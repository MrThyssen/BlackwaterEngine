#include "Blackwater/Input.h"

#include <algorithm>

namespace
{
    // One wheel "notch" on a standard mouse. Matches Win32's WHEEL_DELTA;
    // restated here so this file needs no Windows header.
    constexpr float kWheelUnitsPerNotch = 120.0f;
}

namespace bw
{
    // ----------------------------------------------------------------------
    // Queries
    // ----------------------------------------------------------------------

    bool Input::IsKeyDown(uint8_t virtualKey) const noexcept
    {
        return _keysDown[virtualKey];
    }

    bool Input::WasKeyPressed(uint8_t virtualKey) const noexcept
    {
        return _keysPressed[virtualKey];
    }

    bool Input::IsButtonDown(MouseButton button) const noexcept
    {
        return _buttonsDown[Index(button)];
    }

    bool Input::WasButtonPressed(MouseButton button) const noexcept
    {
        return _buttonsPressed[Index(button)];
    }

    bool Input::WasButtonReleased(MouseButton button) const noexcept
    {
        return _buttonsReleased[Index(button)];
    }

    bool Input::AnyButtonDown() const noexcept
    {
        return std::ranges::any_of(_buttonsDown, [](bool down) { return down; });
    }

    float Input::WheelNotches() const noexcept
    {
        return static_cast<float>(_wheelRaw) / kWheelUnitsPerNotch;
    }

    // ----------------------------------------------------------------------
    // Feeders
    // ----------------------------------------------------------------------

    void Input::OnKey(uint8_t virtualKey, bool down) noexcept
    {
        // Holding a key makes Windows send WM_KEYDOWN repeatedly (autorepeat).
        // "Pressed" must mean the up -> down transition only, so a repeat --
        // a down while already down -- is ignored for edge purposes.
        if (down && !_keysDown[virtualKey])
        {
            _keysPressed[virtualKey] = true;
        }

        _keysDown[virtualKey] = down;
    }

    void Input::OnMouseButton(MouseButton button, bool down) noexcept
    {
        const size_t i = Index(button);

        // Pressed and released are separate flags, not derived from IsDown.
        // A quick click can go down AND up between two simulation steps; the
        // next update then sees IsDown == false, but must still see the press.
        if (down && !_buttonsDown[i])
        {
            _buttonsPressed[i] = true;
        }
        else if (!down && _buttonsDown[i])
        {
            _buttonsReleased[i] = true;
        }

        _buttonsDown[i] = down;
    }

    void Input::OnMouseMove(int32_t x, int32_t y) noexcept
    {
        // The very first move has no previous position to measure from;
        // reporting a delta from (0, 0) would make the camera lurch on startup.
        if (_hasMousePosition)
        {
            // Accumulate rather than overwrite: several moves can arrive
            // between two updates, and the update wants their sum.
            _mouseDeltaX += x - _mouseX;
            _mouseDeltaY += y - _mouseY;
        }

        _mouseX = x;
        _mouseY = y;
        _hasMousePosition = true;
    }

    void Input::OnMouseWheel(int32_t rawDelta) noexcept
    {
        _wheelRaw += rawDelta;
    }

    void Input::OnFocusLost() noexcept
    {
        _keysDown.fill(false);
        _buttonsDown.fill(false);

        // Pending edges are dropped too: an action triggered by a press the
        // user made just before switching away would be a surprise.
        _keysPressed.fill(false);
        _buttonsPressed.fill(false);
        _buttonsReleased.fill(false);
    }

    void Input::EndUpdate() noexcept
    {
        _keysPressed.fill(false);
        _buttonsPressed.fill(false);
        _buttonsReleased.fill(false);

        _mouseDeltaX = 0;
        _mouseDeltaY = 0;
        _wheelRaw    = 0;
    }
}
